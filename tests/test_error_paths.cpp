#include <cassert>
#include <vector>

#include "wayvault/container.h"
#include "wayvault/sample_builder.h"
#include "wayvault/tile_decoder.h"

int main() {
  bool threw = false;
  try {
    const std::vector<unsigned char> bad = {'N', 'O', 'P', 'E'};
    (void)wayvault::parse_vault(bad.data(), bad.size());
  } catch (const wayvault::ParseError&) {
    threw = true;
  }
  assert(threw);

  wayvault::SampleTileSpec tile;
  tile.id = 99;
  tile.name = "short-raw";
  tile.width = 4;
  tile.height = 4;
  tile.codec = 0;
  tile.payload = {1, 2, 3};
  std::vector<unsigned char> sample = wayvault::build_sample_vault({tile}, {}, false);
  wayvault::VaultFile file = wayvault::parse_vault(sample.data(), sample.size());

  threw = false;
  try {
    (void)wayvault::decode_tile(file, file.tiles.front());
  } catch (const wayvault::DecodeError&) {
    threw = true;
  }
  assert(threw);
  return 0;
}
