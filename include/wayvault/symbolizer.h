#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace wayvault {

struct SymbolRule {
  std::string key;
  std::string value;
  std::string symbol;
  uint32_t priority = 0;
};

struct SymbolizedFeature {
  std::string feature_id;
  std::string symbol;
  uint32_t priority = 0;
  bool fallback = false;
};

class Symbolizer {
 public:
  void add_rule(SymbolRule rule);
  SymbolizedFeature symbolize(const std::string& feature_id, const std::map<std::string, std::string>& tags) const;
  std::vector<SymbolRule> rules() const { return rules_; }
 private:
  std::vector<SymbolRule> rules_;
};

Symbolizer parse_symbolizer(const std::string& text);
std::string write_symbolizer(const Symbolizer& symbolizer);
std::vector<SymbolizedFeature> symbolize_many(const Symbolizer& symbolizer, const std::map<std::string, std::map<std::string, std::string>>& features);
std::map<std::string, size_t> count_symbols(const std::vector<SymbolizedFeature>& features);
uint32_t symbolizer_hash(const std::vector<SymbolizedFeature>& features);

}  // namespace wayvault
