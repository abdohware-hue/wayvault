#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "wayvault/container.h"

namespace wayvault {

struct MetadataSummary {
  std::map<std::string, std::string> values;
  uint32_t flags_or = 0;
  uint32_t stable_hash = 0;
};

MetadataSummary summarize_metadata(const VaultFile& file);
std::vector<std::string> collect_labels(const VaultFile& file);
std::string metadata_report(const VaultFile& file);

}  // namespace wayvault
