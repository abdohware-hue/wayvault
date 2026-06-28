#include "wayvault/render.h"

#include <algorithm>
#include <sstream>

#include "wayvault/checksum.h"

namespace wayvault {

std::vector<uint8_t> downsample_2x(const DecodedTile& tile) {
  if (tile.width == 0 || tile.height == 0 || tile.pixels.empty()) {
    return {};
  }
  size_t out_w = std::max<size_t>(1, tile.width / 2);
  size_t out_h = std::max<size_t>(1, tile.height / 2);
  std::vector<uint8_t> out(out_w * out_h, 0);
  for (size_t y = 0; y < out_h; ++y) {
    for (size_t x = 0; x < out_w; ++x) {
      size_t src_x = std::min<size_t>(tile.width - 1, x * 2);
      size_t src_y = std::min<size_t>(tile.height - 1, y * 2);
      out[y * out_w + x] = tile.pixels[src_y * tile.width + src_x];
    }
  }
  return out;
}

std::string ascii_preview(const DecodedTile& tile, size_t max_width, size_t max_height) {
  static const char shades[] = " .:-=+*#%@";
  if (tile.width == 0 || tile.height == 0 || tile.pixels.empty()) {
    return "";
  }
  size_t w = std::min<size_t>(tile.width, max_width);
  size_t h = std::min<size_t>(tile.height, max_height);
  std::ostringstream out;
  for (size_t y = 0; y < h; ++y) {
    for (size_t x = 0; x < w; ++x) {
      uint8_t value = tile.pixels[y * tile.width + x];
      out << shades[(value * 9) / 255];
    }
    if (y + 1 < h) {
      out << '\n';
    }
  }
  return out.str();
}

uint32_t compare_tiles(const DecodedTile& left, const DecodedTile& right) {
  size_t common = std::min(left.pixels.size(), right.pixels.size());
  uint32_t state = rotate_mix(left.id, right.id);
  for (size_t i = 0; i < common; ++i) {
    state = rotate_mix(state, static_cast<uint32_t>(left.pixels[i] ^ right.pixels[i]));
  }
  state = rotate_mix(state, static_cast<uint32_t>(left.pixels.size()));
  state = rotate_mix(state, static_cast<uint32_t>(right.pixels.size()));
  return state;
}

}  // namespace wayvault
