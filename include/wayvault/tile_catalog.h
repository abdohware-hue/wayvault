#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "wayvault/container.h"

namespace wayvault {

struct CatalogTile {
  uint32_t id = 0;
  uint8_t layer = 0;
  uint8_t codec = 0;
  uint32_t pixels = 0;
  std::string name;
};

struct CatalogSummary {
  size_t tile_count = 0;
  std::map<uint8_t, size_t> by_layer;
  std::map<uint8_t, size_t> by_codec;
  uint64_t total_pixels = 0;
};

class TileCatalog {
 public:
  void add(CatalogTile tile);
  bool remove(uint32_t id);
  const CatalogTile* find(uint32_t id) const;
  std::vector<CatalogTile> by_layer(uint8_t layer) const;
  std::vector<CatalogTile> by_codec(uint8_t codec) const;
  CatalogSummary summary() const;
  std::vector<CatalogTile> all_tiles() const;

 private:
  std::map<uint32_t, CatalogTile> tiles_;
};

TileCatalog build_tile_catalog(const VaultFile& file);
std::string render_catalog_summary(const CatalogSummary& summary);
uint32_t catalog_hash(const TileCatalog& catalog);

}  // namespace wayvault
