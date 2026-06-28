#include "wayvault/resource_quota.h"

#include <sstream>

#include "wayvault/checksum.h"

namespace wayvault {

namespace {

std::vector<std::string> split(const std::string& line, char sep) {
  std::vector<std::string> out;
  std::string cur;
  for (char c : line) {
    if (c == sep) {
      out.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  out.push_back(cur);
  return out;
}

}  // namespace

std::vector<QuotaLimit> parse_quota_limits(const std::string& text) {
  std::vector<QuotaLimit> limits;
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') continue;
    auto cells = split(line, ',');
    if (cells.size() < 3) continue;
    QuotaLimit limit;
    limit.group = cells[0];
    limit.max_bytes = std::stoull(cells[1]);
    limit.max_items = static_cast<uint32_t>(std::stoul(cells[2]));
    if (cells.size() > 3) limit.weight = static_cast<uint32_t>(std::stoul(cells[3]));
    limits.push_back(limit);
  }
  return limits;
}

std::vector<QuotaUsage> parse_quota_usage(const std::string& text) {
  std::vector<QuotaUsage> usage;
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') continue;
    auto cells = split(line, ',');
    if (cells.size() < 4) continue;
    usage.push_back({cells[0], cells[1], std::stoull(cells[2]), static_cast<uint32_t>(std::stoul(cells[3]))});
  }
  return usage;
}

std::map<std::string, uint64_t> quota_bytes_by_group(const std::vector<QuotaUsage>& usage) {
  std::map<std::string, uint64_t> totals;
  for (const QuotaUsage& row : usage) totals[row.group] += row.bytes;
  return totals;
}

std::vector<QuotaDecision> evaluate_quota(const std::vector<QuotaUsage>& usage, const std::vector<QuotaLimit>& limits) {
  std::map<std::string, QuotaLimit> limit_map;
  for (const QuotaLimit& limit : limits) limit_map[limit.group] = limit;
  std::map<std::string, uint64_t> bytes;
  std::map<std::string, uint32_t> items;
  std::vector<QuotaDecision> decisions;
  for (const QuotaUsage& row : usage) {
    QuotaDecision decision;
    decision.usage = row;
    auto lit = limit_map.find(row.group);
    if (lit == limit_map.end()) {
      decision.accepted = true;
      decision.reason = "unlimited";
    } else {
      const QuotaLimit& limit = lit->second;
      uint64_t next_bytes = bytes[row.group] + row.bytes;
      uint32_t next_items = items[row.group] + row.items;
      if (limit.max_bytes && next_bytes > limit.max_bytes) {
        decision.accepted = false;
        decision.reason = "byte limit";
      } else if (limit.max_items && next_items > limit.max_items) {
        decision.accepted = false;
        decision.reason = "item limit";
      } else {
        decision.accepted = true;
        decision.reason = "within limit";
        bytes[row.group] = next_bytes;
        items[row.group] = next_items;
      }
    }
    decisions.push_back(std::move(decision));
  }
  return decisions;
}

std::string summarize_quota(const std::vector<QuotaDecision>& decisions) {
  size_t accepted = 0;
  size_t rejected = 0;
  std::map<std::string, size_t> reasons;
  for (const QuotaDecision& decision : decisions) {
    decision.accepted ? ++accepted : ++rejected;
    reasons[decision.reason]++;
  }
  std::ostringstream out;
  out << "accepted=" << accepted << " rejected=" << rejected;
  for (const auto& kv : reasons) out << ' ' << kv.first << '=' << kv.second;
  return out.str();
}

uint32_t quota_hash(const std::vector<QuotaDecision>& decisions) {
  uint32_t state = rotate_mix(0xe0051a7du, static_cast<uint32_t>(decisions.size()));
  for (const QuotaDecision& decision : decisions) {
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(decision.usage.group.data()), decision.usage.group.size()));
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(decision.usage.key.data()), decision.usage.key.size()));
    state = rotate_mix(state, static_cast<uint32_t>(decision.usage.bytes));
    state = rotate_mix(state, decision.usage.items);
    state = rotate_mix(state, decision.accepted ? 1u : 0u);
  }
  return state;
}

}  // namespace wayvault
