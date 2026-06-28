#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "wayvault/raster_ops.h"
#include "wayvault/tile_decoder.h"

namespace wayvault {

struct SampleGrid {
  uint32_t columns = 0;
  uint32_t rows = 0;
  std::vector<uint8_t> values;
};

SampleGrid sample_tile_grid(const DecodedTile& tile, uint32_t columns, uint32_t rows);
GrayImage tile_to_image(const DecodedTile& tile);
std::vector<GrayImage> build_mipmap_chain(const GrayImage& image, uint32_t min_size);
uint8_t sample_grid_value(const SampleGrid& grid, uint32_t column, uint32_t row);
uint32_t sample_grid_hash(const SampleGrid& grid);

}  // namespace wayvault
