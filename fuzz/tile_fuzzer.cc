#include <cstddef>
#include <cstdint>

#include "wayvault/container.h"
#include "wayvault/render.h"
#include "wayvault/tile_decoder.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  try {
    wayvault::VaultFile file = wayvault::parse_vault(data, size);
    auto tiles = wayvault::decode_all_tiles(file, 16);
    for (const auto& tile : tiles) {
      (void)wayvault::summarize_tile(tile);
      (void)wayvault::downsample_2x(tile);
    }
  } catch (const wayvault::ParseError&) {
  }
  return 0;
}
