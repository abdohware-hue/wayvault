#include <cassert>
#include <vector>

#include "wayvault/container.h"
#include "wayvault/index.h"
#include "wayvault/layout.h"
#include "wayvault/manifest.h"
#include "wayvault/query.h"
#include "wayvault/stats.h"
#include "wayvault/string_table.h"
#include "wayvault/validator.h"
#include "wayvault/metadata.h"
#include "wayvault/sample_builder.h"
#include "wayvault/stream.h"
#include "wayvault/tile_decoder.h"

int main() {
  std::vector<unsigned char> sample = wayvault::build_default_sample();
  wayvault::VaultFile file = wayvault::parse_vault(sample.data(), sample.size());
  assert(file.version == 1);
  assert(file.tiles.size() == 3);
  assert(file.strings.size() >= 3);

  wayvault::VaultIndex index = wayvault::build_index(file);
  assert(index.by_id.size() == 3);
  assert(index.by_id.count(1) == 1);

  wayvault::DecodedTile tile = wayvault::decode_tile(file, file.tiles.front());
  assert(tile.pixels.size() == 16);
  assert(wayvault::summarize_tile(tile) != 0);

  wayvault::MetadataSummary summary = wayvault::summarize_metadata(file);
  assert(summary.values.count("author") == 1);

  wayvault::DiagnosticSet diagnostics = wayvault::validate_vault(file);
  assert(!diagnostics.has_errors());
  assert(wayvault::compute_tile_stats(file).tile_count == 3);
  assert(!wayvault::layout_report(file).empty());
  assert(!wayvault::export_manifest(file).empty());
  assert(wayvault::string_table_digest(file) != 0);
  auto selected = wayvault::select_tiles(file, wayvault::parse_query("name~tile"));
  assert(selected.size() == 3);

  wayvault::StreamState state = wayvault::process_session(sample.data(), sample.size());
  assert(state.checksum != 0);
  assert(state.decoded_tiles > 0);
  return 0;
}
