#include "wayvault/symbolizer.h"

#include <algorithm>
#include <sstream>

#include "wayvault/checksum.h"

namespace wayvault {

namespace {

std::map<std::string, std::string> parse_pairs(const std::string& line) {
  std::map<std::string, std::string> out;
  std::istringstream input(line);
  std::string token;
  while (input >> token) {
    size_t eq = token.find('=');
    if (eq != std::string::npos) out[token.substr(0, eq)] = token.substr(eq + 1);
  }
  return out;
}

}  // namespace

void Symbolizer::add_rule(SymbolRule rule) {
  rules_.push_back(std::move(rule));
  std::stable_sort(rules_.begin(), rules_.end(), [](const SymbolRule& a, const SymbolRule& b) {
    return a.priority > b.priority;
  });
}

SymbolizedFeature Symbolizer::symbolize(const std::string& feature_id, const std::map<std::string, std::string>& tags) const {
  for (const SymbolRule& rule : rules_) {
    auto it = tags.find(rule.key);
    if (it == tags.end()) continue;
    if (!rule.value.empty() && it->second != rule.value) continue;
    return {feature_id, rule.symbol, rule.priority, false};
  }
  auto kind = tags.find("kind");
  if (kind != tags.end()) return {feature_id, "generic-" + kind->second, 0, true};
  return {feature_id, "generic", 0, true};
}

Symbolizer parse_symbolizer(const std::string& text) {
  Symbolizer symbolizer;
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') continue;
    auto pairs = parse_pairs(line);
    SymbolRule rule;
    rule.key = pairs["key"];
    rule.value = pairs["value"];
    rule.symbol = pairs.count("symbol") ? pairs["symbol"] : "generic";
    rule.priority = pairs.count("priority") ? static_cast<uint32_t>(std::stoul(pairs["priority"])) : 0;
    if (!rule.key.empty()) symbolizer.add_rule(std::move(rule));
  }
  return symbolizer;
}

std::string write_symbolizer(const Symbolizer& symbolizer) {
  std::ostringstream out;
  for (const SymbolRule& rule : symbolizer.rules()) {
    out << "key=" << rule.key;
    if (!rule.value.empty()) out << " value=" << rule.value;
    out << " symbol=" << rule.symbol << " priority=" << rule.priority << '\n';
  }
  return out.str();
}

std::vector<SymbolizedFeature> symbolize_many(const Symbolizer& symbolizer, const std::map<std::string, std::map<std::string, std::string>>& features) {
  std::vector<SymbolizedFeature> out;
  for (const auto& kv : features) out.push_back(symbolizer.symbolize(kv.first, kv.second));
  std::sort(out.begin(), out.end(), [](const SymbolizedFeature& a, const SymbolizedFeature& b) {
    if (a.priority != b.priority) return a.priority > b.priority;
    return a.feature_id < b.feature_id;
  });
  return out;
}

std::map<std::string, size_t> count_symbols(const std::vector<SymbolizedFeature>& features) {
  std::map<std::string, size_t> counts;
  for (const SymbolizedFeature& feature : features) counts[feature.symbol]++;
  return counts;
}

uint32_t symbolizer_hash(const std::vector<SymbolizedFeature>& features) {
  uint32_t state = rotate_mix(0xc4b23e11u, static_cast<uint32_t>(features.size()));
  for (const SymbolizedFeature& feature : features) {
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(feature.feature_id.data()), feature.feature_id.size()));
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(feature.symbol.data()), feature.symbol.size()));
    state = rotate_mix(state, feature.priority);
    state = rotate_mix(state, feature.fallback ? 1u : 0u);
  }
  return state;
}

}  // namespace wayvault
