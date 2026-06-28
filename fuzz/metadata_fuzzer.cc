#include <cstddef>
#include <cstdint>

#include "wayvault/container.h"
#include "wayvault/metadata.h"
#include "wayvault/query.h"
#include "wayvault/string_table.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  try {
    wayvault::VaultFile file = wayvault::parse_vault(data, size);
    auto labels = wayvault::collect_labels(file);
    auto report = wayvault::metadata_report(file);
    auto query = wayvault::parse_query("name~tile area>3");
    auto selected = wayvault::select_tiles(file, query);
    auto unique = wayvault::unique_normalized_strings(file);
    (void)labels;
    (void)report;
    (void)selected;
    (void)unique;
  } catch (const wayvault::ParseError&) {
  }
  return 0;
}
