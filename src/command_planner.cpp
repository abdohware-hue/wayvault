#include "wayvault/command_planner.h"

#include "wayvault/checksum.h"
#include "wayvault/index.h"

namespace wayvault {

std::vector<StreamCommand> plan_preview_walk(const VaultFile& file, size_t max_tiles) {
  std::vector<StreamCommand> commands;
  std::vector<uint32_t> ids = sorted_tile_ids(file);
  if (ids.empty()) {
    return commands;
  }
  size_t n = std::min(max_tiles, ids.size());
  for (size_t i = 0; i < n; ++i) {
    commands.push_back(StreamCommand{1, ids[i], static_cast<uint16_t>(i), 4});
    if (i == 0) {
      commands.push_back(StreamCommand{2, ids[i], 0, 4});
    }
  }
  commands.push_back(StreamCommand{3, 0, 0, 8});
  return commands;
}

std::vector<StreamCommand> plan_layer_probe(const VaultFile& file, uint8_t layer) {
  std::vector<StreamCommand> commands;
  for (const TileRecord& tile : file.tiles) {
    if (tile.layer == layer) {
      commands.push_back(StreamCommand{1, tile.id, tile.width, 4});
    }
  }
  return commands;
}

uint32_t command_plan_digest(const std::vector<StreamCommand>& commands) {
  uint32_t state = 0x55667788u;
  for (const StreamCommand& command : commands) {
    state = rotate_mix(state, command.op);
    state = rotate_mix(state, command.tile_id);
    state = rotate_mix(state, command.arg);
    state = rotate_mix(state, command.repeat);
  }
  return state;
}

}  // namespace wayvault
