#include "wayvault/stream.h"

#include "wayvault/byte_reader.h"
#include "wayvault/checksum.h"
#include "wayvault/index.h"

namespace wayvault {

std::vector<StreamCommand> parse_commands(const VaultFile& file) {
  const SectionInfo* section = find_section(file, TAG_CMDS);
  if (section == nullptr) {
    return {};
  }
  ByteReader reader(section_data(file, *section), section->size);
  uint16_t count = reader.remaining() >= 2 ? reader.read_u16() : 0;
  std::vector<StreamCommand> commands;
  commands.reserve(count);
  for (uint16_t i = 0; i < count; ++i) {
    if (reader.remaining() < 8) {
      throw ParseError("truncated command stream");
    }
    StreamCommand command;
    command.op = reader.read_u8();
    command.tile_id = reader.read_u32();
    command.arg = reader.read_u16();
    command.repeat = reader.read_u8();
    commands.push_back(command);
  }
  return commands;
}

StreamState execute_commands(const VaultFile& file, const std::vector<StreamCommand>& commands) {
  size_t capacity = 2 + (file.flags & 1u);
  TileCache cache(capacity);
  StreamState state;
  state.checksum = 0x76543210u;

  std::vector<uint32_t> known_ids = sorted_tile_ids(file);
  for (const StreamCommand& command : commands) {
    switch (command.op) {
      case 1: {
        TileView view = cache.view_for(file, command.tile_id);
        state.checksum = rotate_mix(state.checksum, sample_view_bytes(view, command.arg, 8));
        ++state.decoded_tiles;
        break;
      }
      case 2: {
        state.retained = cache.view_for(file, command.tile_id);
        state.checksum = rotate_mix(state.checksum, state.retained.tile_id);
        break;
      }
      case 3: {
        state.checksum = rotate_mix(state.checksum, sample_view_bytes(state.retained, command.arg, command.repeat));
        ++state.sampled_views;
        break;
      }
      case 4: {
        uint8_t repeat = command.repeat == 0 ? 1 : command.repeat;
        for (uint8_t i = 0; i < repeat; ++i) {
          uint32_t id = command.tile_id + i;
          if (!known_ids.empty()) {
            id = known_ids[(command.arg + i) % known_ids.size()];
          }
          TileView view = cache.view_for(file, id);
          state.checksum = rotate_mix(state.checksum, sample_view_bytes(view, command.arg + i, 4));
          ++state.decoded_tiles;
        }
        break;
      }
      case 5: {
        state.decoded_tiles = 0;
        state.sampled_views = 0;
        break;
      }
      default:
        state.checksum = rotate_mix(state.checksum, command.op);
        break;
    }
  }
  return state;
}

StreamState process_session(const uint8_t* data, size_t size) {
  VaultFile file = parse_vault(data, size);
  std::vector<StreamCommand> commands = parse_commands(file);
  return execute_commands(file, commands);
}

StreamState process_session(const std::vector<uint8_t>& bytes) {
  return process_session(bytes.data(), bytes.size());
}

}  // namespace wayvault
