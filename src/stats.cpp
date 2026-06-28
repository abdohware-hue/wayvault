#include "wayvault/stats.h"

#include <algorithm>
#include <sstream>

#include "wayvault/checksum.h"

namespace wayvault {

TileStats compute_tile_stats(const VaultFile& file) {
  TileStats stats;
  stats.tile_count = file.tiles.size();
  uint32_t state = 0x2468ace0u;
  for (const TileRecord& tile : file.tiles) {
    stats.total_pixels += static_cast<uint64_t>(tile.width) * static_cast<uint64_t>(tile.height);
    stats.total_payload_bytes += tile.data_size;
    stats.max_width = std::max(stats.max_width, tile.width);
    stats.max_height = std::max(stats.max_height, tile.height);
    stats.codecs[tile.codec] += 1;
    state = rotate_mix(state, tile.id);
    state = rotate_mix(state, static_cast<uint32_t>(tile.width) << 16 | tile.height);
    state = rotate_mix(state, tile.data_size);
    state = rotate_mix(state, tile.codec);
  }
  stats.fingerprint = state;
  return stats;
}

std::string stats_report(const VaultFile& file) {
  TileStats stats = compute_tile_stats(file);
  std::ostringstream out;
  out << "tiles=" << stats.tile_count << " pixels=" << stats.total_pixels
      << " payload=" << stats.total_payload_bytes << " max=" << stats.max_width << "x"
      << stats.max_height << " fingerprint=" << stats.fingerprint;
  for (const auto& kv : stats.codecs) {
    out << " codec" << static_cast<int>(kv.first) << "=" << kv.second;
  }
  return out.str();
}

}  // namespace wayvault
