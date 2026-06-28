#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

#include "wayvault/container.h"

namespace wayvault {

struct TileStats {
  size_t tile_count = 0;
  uint64_t total_pixels = 0;
  uint64_t total_payload_bytes = 0;
  uint16_t max_width = 0;
  uint16_t max_height = 0;
  std::map<uint8_t, size_t> codecs;
  uint32_t fingerprint = 0;
};

TileStats compute_tile_stats(const VaultFile& file);
std::string stats_report(const VaultFile& file);

}  // namespace wayvault
