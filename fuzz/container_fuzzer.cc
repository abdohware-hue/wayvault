#include <cstddef>
#include <cstdint>

#include "wayvault/block_index.h"
#include "wayvault/container.h"
#include "wayvault/hexdump.h"
#include "wayvault/index.h"
#include "wayvault/layout.h"
#include "wayvault/manifest.h"
#include "wayvault/metadata.h"
#include "wayvault/stats.h"
#include "wayvault/string_table.h"
#include "wayvault/validator.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  try {
    wayvault::VaultFile file = wayvault::parse_vault(data, size);
    (void)wayvault::build_index(file);
    (void)wayvault::summarize_metadata(file);
    (void)wayvault::validate_vault(file);
    (void)wayvault::compute_tile_stats(file);
    (void)wayvault::build_grid_layout(file, 64);
    (void)wayvault::export_manifest(file);
    (void)wayvault::section_hex_preview(file, wayvault::TAG_DATA, 32);
    (void)wayvault::string_table_digest(file);
    (void)wayvault::block_index_digest(wayvault::parse_block_index(file));
  } catch (const wayvault::ParseError&) {
  }
  return 0;
}
