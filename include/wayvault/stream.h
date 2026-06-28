#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "wayvault/container.h"
#include "wayvault/tile_cache.h"

namespace wayvault {

struct StreamCommand {
  uint8_t op = 0;
  uint32_t tile_id = 0;
  uint16_t arg = 0;
  uint8_t repeat = 0;
};

struct StreamState {
  uint32_t checksum = 0;
  uint32_t decoded_tiles = 0;
  uint32_t sampled_views = 0;
  TileView retained;
};

std::vector<StreamCommand> parse_commands(const VaultFile& file);
StreamState execute_commands(const VaultFile& file, const std::vector<StreamCommand>& commands);
StreamState process_session(const uint8_t* data, size_t size);
StreamState process_session(const std::vector<uint8_t>& bytes);

}  // namespace wayvault
