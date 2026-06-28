#include "wayvault/retention_policy.h"

#include <algorithm>
#include <sstream>

#include "wayvault/checksum.h"

namespace wayvault {

namespace {

std::vector<std::string> split_ws(const std::string& line) {
  std::istringstream input(line);
  std::vector<std::string> out;
  std::string token;
  while (input >> token) out.push_back(token);
  return out;
}

bool applies(const RetentionRule& rule, const RetentionCandidate& candidate) {
  if (!rule.prefix.empty() && candidate.key.compare(0, rule.prefix.size(), rule.prefix) != 0) return false;
  if (candidate.zoom < rule.min_zoom || candidate.zoom > rule.max_zoom) return false;
  return true;
}

}  // namespace

std::vector<RetentionRule> parse_retention_rules(const std::string& text) {
  std::vector<RetentionRule> rules;
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') continue;
    auto tokens = split_ws(line);
    RetentionRule rule;
    for (const std::string& token : tokens) {
      size_t eq = token.find('=');
      std::string key = eq == std::string::npos ? token : token.substr(0, eq);
      std::string value = eq == std::string::npos ? "1" : token.substr(eq + 1);
      if (key == "prefix") rule.prefix = value;
      else if (key == "min_zoom") rule.min_zoom = static_cast<uint32_t>(std::stoul(value));
      else if (key == "max_zoom") rule.max_zoom = static_cast<uint32_t>(std::stoul(value));
      else if (key == "max_age") rule.max_age_seconds = std::stoull(value);
      else if (key == "min_keep") rule.min_keep_bytes = std::stoull(value);
      else if (key == "pinned") rule.pinned = value != "0" && value != "false";
    }
    if (rule.min_zoom > rule.max_zoom) std::swap(rule.min_zoom, rule.max_zoom);
    rules.push_back(rule);
  }
  return rules;
}

RetentionDecision evaluate_retention(const RetentionCandidate& candidate, const std::vector<RetentionRule>& rules) {
  RetentionDecision decision;
  decision.candidate = candidate;
  if (candidate.referenced) {
    decision.keep = true;
    decision.reason = "referenced";
    return decision;
  }
  for (const RetentionRule& rule : rules) {
    if (!applies(rule, candidate)) continue;
    if (rule.pinned) {
      decision.keep = true;
      decision.reason = "pinned rule";
      return decision;
    }
    if (rule.max_age_seconds && candidate.age_seconds <= rule.max_age_seconds) {
      decision.keep = true;
      decision.reason = "fresh";
      return decision;
    }
    if (rule.min_keep_bytes && candidate.size <= rule.min_keep_bytes) {
      decision.keep = true;
      decision.reason = "small protected";
      return decision;
    }
  }
  decision.keep = false;
  decision.reason = "expired";
  return decision;
}

std::vector<RetentionDecision> plan_retention(const std::vector<RetentionCandidate>& candidates, const std::vector<RetentionRule>& rules, uint64_t target_free_bytes) {
  std::vector<RetentionDecision> decisions;
  decisions.reserve(candidates.size());
  for (const RetentionCandidate& candidate : candidates) decisions.push_back(evaluate_retention(candidate, rules));
  std::sort(decisions.begin(), decisions.end(), [](const RetentionDecision& a, const RetentionDecision& b) {
    if (a.keep != b.keep) return !a.keep;
    if (a.candidate.age_seconds != b.candidate.age_seconds) return a.candidate.age_seconds > b.candidate.age_seconds;
    return a.candidate.size > b.candidate.size;
  });
  uint64_t freed = 0;
  for (RetentionDecision& decision : decisions) {
    if (!decision.keep) {
      freed += decision.candidate.size;
      if (freed >= target_free_bytes) break;
    }
  }
  if (freed < target_free_bytes) {
    for (RetentionDecision& decision : decisions) {
      if (!decision.keep || decision.candidate.referenced) continue;
      decision.keep = false;
      decision.reason = "needed for budget";
      freed += decision.candidate.size;
      if (freed >= target_free_bytes) break;
    }
  }
  std::sort(decisions.begin(), decisions.end(), [](const RetentionDecision& a, const RetentionDecision& b) {
    return a.candidate.key < b.candidate.key;
  });
  return decisions;
}

uint64_t retained_bytes(const std::vector<RetentionDecision>& decisions) {
  uint64_t total = 0;
  for (const RetentionDecision& decision : decisions) if (decision.keep) total += decision.candidate.size;
  return total;
}

uint64_t releasable_bytes(const std::vector<RetentionDecision>& decisions) {
  uint64_t total = 0;
  for (const RetentionDecision& decision : decisions) if (!decision.keep) total += decision.candidate.size;
  return total;
}

std::string summarize_retention(const std::vector<RetentionDecision>& decisions) {
  size_t keep = 0;
  size_t drop = 0;
  for (const RetentionDecision& decision : decisions) decision.keep ? ++keep : ++drop;
  std::ostringstream out;
  out << "keep=" << keep << " drop=" << drop << " retained=" << retained_bytes(decisions)
      << " releasable=" << releasable_bytes(decisions);
  return out.str();
}

uint32_t retention_hash(const std::vector<RetentionDecision>& decisions) {
  uint32_t state = rotate_mix(0xb07646d1u, static_cast<uint32_t>(decisions.size()));
  for (const RetentionDecision& decision : decisions) {
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(decision.candidate.key.data()), decision.candidate.key.size()));
    state = rotate_mix(state, decision.candidate.zoom);
    state = rotate_mix(state, static_cast<uint32_t>(decision.candidate.size));
    state = rotate_mix(state, decision.keep ? 1u : 0u);
  }
  return state;
}

}  // namespace wayvault
