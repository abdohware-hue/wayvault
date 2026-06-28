#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "wayvault/container.h"

namespace wayvault {

struct DiffItem {
  std::string path;
  std::string before;
  std::string after;
  int severity = 0;
};

struct VaultDiff {
  std::vector<DiffItem> items;
  uint32_t before_hash = 0;
  uint32_t after_hash = 0;
};

VaultDiff diff_vaults(const VaultFile& before, const VaultFile& after);
std::string format_diff(const VaultDiff& diff);
uint32_t diff_hash(const VaultDiff& diff);
std::vector<DiffItem> filter_diff_by_severity(const VaultDiff& diff, int min_severity);

}  // namespace wayvault
