#include "wayvault/manifest.h"

#include <sstream>

#include "wayvault/index.h"
#include "wayvault/stats.h"
#include "wayvault/tags.h"

namespace wayvault {

std::string export_tile_manifest_line(const VaultFile& file, const TileRecord& tile) {
  std::ostringstream out;
  out << "{id:" << tile.id << ",name:\"" << tile_display_name(file, tile) << "\",size:"
      << tile.width << "x" << tile.height << ",codec:" << static_cast<int>(tile.codec)
      << ",payload:" << tile.data_offset << "+" << tile.data_size << "}";
  return out.str();
}

std::string export_manifest(const VaultFile& file) {
  std::ostringstream out;
  TileStats stats = compute_tile_stats(file);
  out << "wayvault { version:" << file.version << ", sections:" << file.sections.size()
      << ", tiles:" << stats.tile_count << ", fingerprint:" << stats.fingerprint << " }\n";
  for (const SectionInfo& section : file.sections) {
    out << "section " << tag_to_string(section.tag) << " offset=" << section.offset
        << " size=" << section.size << "\n";
  }
  for (const TileRecord& tile : file.tiles) {
    out << "tile " << export_tile_manifest_line(file, tile) << "\n";
  }
  return out.str();
}

}  // namespace wayvault
