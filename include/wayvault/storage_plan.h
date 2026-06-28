#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "wayvault/container.h"

namespace wayvault {

struct StorageExtent {
  std::string name;
  uint64_t offset = 0;
  uint64_t size = 0;
  uint32_t alignment = 1;
};

struct StoragePlan {
  std::vector<StorageExtent> extents;
  uint64_t total_size = 0;
  uint64_t slack_bytes = 0;
};

uint64_t align_up_u64(uint64_t value, uint64_t alignment);
StoragePlan plan_storage_for_vault(const VaultFile& file, uint32_t alignment);
StoragePlan compact_storage_plan(std::vector<StorageExtent> extents, uint32_t alignment);
std::string format_storage_plan(const StoragePlan& plan);
uint32_t storage_plan_hash(const StoragePlan& plan);

}  // namespace wayvault
