#include "wayvault/validator.h"

#include <set>
#include <sstream>

#include "wayvault/byte_reader.h"
#include "wayvault/index.h"

namespace wayvault {

DiagnosticSet validate_vault(const VaultFile& file, const ValidationOptions& options) {
  DiagnosticSet out;
  if (file.version == 0 || file.version > 3) {
    out.error("version", "unsupported file version");
  }
  const SectionInfo* data = find_section(file, TAG_DATA);
  if (data == nullptr && options.require_data_section) {
    out.error("missing-data", "DATA section is required for tile payloads");
  }
  if (file.tiles.empty()) {
    out.warning("empty-tiles", "vault contains no tile records");
  }
  if (file.metadata.empty() && options.warn_on_empty_metadata) {
    out.info("empty-metadata", "vault has no metadata entries");
  }

  std::set<uint32_t> seen_ids;
  for (const TileRecord& tile : file.tiles) {
    if (!options.allow_duplicate_tile_ids && !seen_ids.insert(tile.id).second) {
      out.error("duplicate-tile", "duplicate tile identifier", tile.id);
    }
    if (tile.width == 0 || tile.height == 0) {
      out.error("tile-size", "tile has zero width or height", tile.id);
      continue;
    }
    size_t area = 0;
    if (!checked_mul(tile.width, tile.height, &area) || area > options.max_tile_area) {
      out.error("tile-area", "tile area exceeds configured limit", tile.id);
    }
    if (data != nullptr && (tile.data_offset > data->size || tile.data_size > data->size - tile.data_offset)) {
      out.error("payload-range", "tile payload range falls outside DATA section", tile.id);
    }
    if (tile.codec > 3) {
      out.warning("codec", "unknown tile codec", tile.id);
    }
    if (tile.name_index >= file.strings.size()) {
      out.warning("name-index", "tile name index is not present in string table", tile.id);
    }
  }

  for (const MetadataEntry& entry : file.metadata) {
    if (entry.key_index >= file.strings.size()) {
      out.warning("meta-key", "metadata key index is missing");
    }
    if (entry.value_index >= file.strings.size()) {
      out.warning("meta-value", "metadata value index is missing");
    }
  }

  return out;
}

void validate_or_throw(const VaultFile& file, const ValidationOptions& options) {
  DiagnosticSet diagnostics = validate_vault(file, options);
  if (diagnostics.has_errors()) {
    throw ParseError(diagnostics.report());
  }
}

}  // namespace wayvault
