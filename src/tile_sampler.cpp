#include "wayvault/tile_sampler.h"

#include <algorithm>

#include "wayvault/checksum.h"
#include "wayvault/error.h"

namespace wayvault {

GrayImage tile_to_image(const DecodedTile& tile) {
  if (tile.width == 0 || tile.height == 0) throw DecodeError("tile image has zero dimensions");
  if (tile.pixels.size() != static_cast<size_t>(tile.width) * tile.height) throw DecodeError("tile image size mismatch");
  GrayImage image;
  image.width = tile.width;
  image.height = tile.height;
  image.pixels = tile.pixels;
  return image;
}

SampleGrid sample_tile_grid(const DecodedTile& tile, uint32_t columns, uint32_t rows) {
  GrayImage image = tile_to_image(tile);
  SampleGrid grid;
  grid.columns = columns;
  grid.rows = rows;
  grid.values.assign(static_cast<size_t>(columns) * rows, 0);
  if (columns == 0 || rows == 0) return grid;
  for (uint32_t row = 0; row < rows; ++row) {
    uint32_t y0 = static_cast<uint64_t>(row) * image.height / rows;
    uint32_t y1 = std::max<uint32_t>(y0 + 1, static_cast<uint64_t>(row + 1) * image.height / rows);
    y1 = std::min(y1, image.height);
    for (uint32_t col = 0; col < columns; ++col) {
      uint32_t x0 = static_cast<uint64_t>(col) * image.width / columns;
      uint32_t x1 = std::max<uint32_t>(x0 + 1, static_cast<uint64_t>(col + 1) * image.width / columns);
      x1 = std::min(x1, image.width);
      uint32_t sum = 0;
      uint32_t count = 0;
      for (uint32_t y = y0; y < y1; ++y) {
        for (uint32_t x = x0; x < x1; ++x) {
          sum += image.pixels[static_cast<size_t>(y) * image.width + x];
          ++count;
        }
      }
      grid.values[static_cast<size_t>(row) * columns + col] = count == 0 ? 0 : static_cast<uint8_t>(sum / count);
    }
  }
  return grid;
}

std::vector<GrayImage> build_mipmap_chain(const GrayImage& image, uint32_t min_size) {
  std::vector<GrayImage> chain;
  chain.push_back(image);
  while (chain.back().width > min_size || chain.back().height > min_size) {
    uint32_t next_w = std::max<uint32_t>(1, chain.back().width / 2);
    uint32_t next_h = std::max<uint32_t>(1, chain.back().height / 2);
    if (next_w == chain.back().width && next_h == chain.back().height) break;
    chain.push_back(resize_nearest(chain.back(), next_w, next_h));
  }
  return chain;
}

uint8_t sample_grid_value(const SampleGrid& grid, uint32_t column, uint32_t row) {
  if (column >= grid.columns || row >= grid.rows) return 0;
  size_t index = static_cast<size_t>(row) * grid.columns + column;
  return index < grid.values.size() ? grid.values[index] : 0;
}

uint32_t sample_grid_hash(const SampleGrid& grid) {
  uint32_t state = fnv1a32(grid.values);
  state = rotate_mix(state, grid.columns);
  state = rotate_mix(state, grid.rows);
  return state;
}

}  // namespace wayvault
