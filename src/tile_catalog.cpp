#include "wayvault/tile_catalog.h"

#include <algorithm>
#include <sstream>

#include "wayvault/checksum.h"
#include "wayvault/index.h"

namespace wayvault {

void TileCatalog::add(CatalogTile tile) {
  tiles_[tile.id] = std::move(tile);
}

bool TileCatalog::remove(uint32_t id) {
  return tiles_.erase(id) != 0;
}

const CatalogTile* TileCatalog::find(uint32_t id) const {
  auto it = tiles_.find(id);
  if (it == tiles_.end()) return nullptr;
  return &it->second;
}

std::vector<CatalogTile> TileCatalog::by_layer(uint8_t layer) const {
  std::vector<CatalogTile> out;
  for (const auto& kv : tiles_) {
    if (kv.second.layer == layer) out.push_back(kv.second);
  }
  return out;
}

std::vector<CatalogTile> TileCatalog::by_codec(uint8_t codec) const {
  std::vector<CatalogTile> out;
  for (const auto& kv : tiles_) {
    if (kv.second.codec == codec) out.push_back(kv.second);
  }
  return out;
}

CatalogSummary TileCatalog::summary() const {
  CatalogSummary summary;
  summary.tile_count = tiles_.size();
  for (const auto& kv : tiles_) {
    const CatalogTile& tile = kv.second;
    ++summary.by_layer[tile.layer];
    ++summary.by_codec[tile.codec];
    summary.total_pixels += tile.pixels;
  }
  return summary;
}

std::vector<CatalogTile> TileCatalog::all_tiles() const {
  std::vector<CatalogTile> out;
  for (const auto& kv : tiles_) out.push_back(kv.second);
  return out;
}

TileCatalog build_tile_catalog(const VaultFile& file) {
  TileCatalog catalog;
  for (const TileRecord& tile : file.tiles) {
    CatalogTile entry;
    entry.id = tile.id;
    entry.layer = tile.layer;
    entry.codec = tile.codec;
    entry.pixels = static_cast<uint32_t>(tile.width) * static_cast<uint32_t>(tile.height);
    entry.name = tile_display_name(file, tile);
    catalog.add(entry);
  }
  return catalog;
}

std::string render_catalog_summary(const CatalogSummary& summary) {
  std::ostringstream out;
  out << "tiles=" << summary.tile_count << " pixels=" << summary.total_pixels;
  out << "\nlayers";
  for (const auto& kv : summary.by_layer) out << "\n  " << static_cast<int>(kv.first) << "=" << kv.second;
  out << "\ncodecs";
  for (const auto& kv : summary.by_codec) out << "\n  " << static_cast<int>(kv.first) << "=" << kv.second;
  return out.str();
}

uint32_t catalog_hash(const TileCatalog& catalog) {
  uint32_t state = 2166136261u;
  for (const CatalogTile& tile : catalog.all_tiles()) {
    state = rotate_mix(state, tile.id);
    state = rotate_mix(state, tile.pixels);
    state = rotate_mix(state, static_cast<uint32_t>(tile.layer) << 8 | tile.codec);
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(tile.name.data()), tile.name.size()));
  }
  return state;
}

}  // namespace wayvault
