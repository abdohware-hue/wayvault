#include "wayvault/feature_flags.h"

#include <sstream>

#include "wayvault/error.h"

namespace wayvault {

void FeatureRegistry::define(FeatureFlag flag) {
  flags_[flag.name] = flag;
}

uint32_t FeatureRegistry::parse_enabled_mask(const std::vector<std::string>& names) const {
  uint32_t mask = 0;
  for (const std::string& name : names) {
    auto it = flags_.find(name);
    if (it == flags_.end()) throw ParseError("unknown feature flag: " + name);
    mask |= it->second.bit;
  }
  return mask;
}

std::vector<std::string> FeatureRegistry::names_for_mask(uint32_t mask) const {
  std::vector<std::string> out;
  for (const auto& kv : flags_) {
    if (mask & kv.second.bit) out.push_back(kv.first);
  }
  return out;
}

bool FeatureRegistry::is_known(const std::string& name) const {
  return flags_.count(name) != 0;
}

FeatureRegistry default_feature_registry() {
  FeatureRegistry registry;
  registry.define(FeatureFlag{"string-table", 1u << 0, true});
  registry.define(FeatureFlag{"command-stream", 1u << 1, true});
  registry.define(FeatureFlag{"tile-index", 1u << 2, false});
  registry.define(FeatureFlag{"metadata", 1u << 3, true});
  registry.define(FeatureFlag{"atlas", 1u << 4, false});
  registry.define(FeatureFlag{"journal", 1u << 5, false});
  return registry;
}

uint32_t parse_feature_list(const FeatureRegistry& registry, const std::string& text) {
  std::vector<std::string> names;
  std::stringstream ss(text);
  std::string item;
  while (std::getline(ss, item, ',')) {
    if (!item.empty()) names.push_back(item);
  }
  return registry.parse_enabled_mask(names);
}

std::string format_feature_mask(const FeatureRegistry& registry, uint32_t mask) {
  std::ostringstream out;
  bool first = true;
  for (const std::string& name : registry.names_for_mask(mask)) {
    if (!first) out << ',';
    first = false;
    out << name;
  }
  return out.str();
}

}  // namespace wayvault
