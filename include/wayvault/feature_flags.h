#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace wayvault {

struct FeatureFlag {
  std::string name;
  uint32_t bit = 0;
  bool enabled_by_default = false;
};

class FeatureRegistry {
 public:
  void define(FeatureFlag flag);
  uint32_t parse_enabled_mask(const std::vector<std::string>& names) const;
  std::vector<std::string> names_for_mask(uint32_t mask) const;
  bool is_known(const std::string& name) const;
  size_t size() const { return flags_.size(); }

 private:
  std::map<std::string, FeatureFlag> flags_;
};

FeatureRegistry default_feature_registry();
uint32_t parse_feature_list(const FeatureRegistry& registry, const std::string& text);
std::string format_feature_mask(const FeatureRegistry& registry, uint32_t mask);

}  // namespace wayvault
