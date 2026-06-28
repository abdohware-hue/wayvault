#include "wayvault/raster_ops.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "wayvault/checksum.h"
#include "wayvault/error.h"

namespace wayvault {

namespace {

size_t pixel_index(const GrayImage& image, uint32_t x, uint32_t y) {
  return static_cast<size_t>(y) * image.width + x;
}

void require_shape(const GrayImage& image) {
  if (image.width == 0 || image.height == 0) throw DecodeError("image has zero dimensions");
  if (image.pixels.size() != static_cast<size_t>(image.width) * image.height) {
    throw DecodeError("image buffer does not match dimensions");
  }
}

}  // namespace

GrayImage make_image(uint32_t width, uint32_t height, uint8_t fill) {
  GrayImage image;
  image.width = width;
  image.height = height;
  image.pixels.assign(static_cast<size_t>(width) * height, fill);
  return image;
}

uint8_t sample_nearest(const GrayImage& image, int x, int y, uint8_t outside) {
  require_shape(image);
  if (x < 0 || y < 0 || x >= static_cast<int>(image.width) || y >= static_cast<int>(image.height)) return outside;
  return image.pixels[pixel_index(image, static_cast<uint32_t>(x), static_cast<uint32_t>(y))];
}

GrayImage resize_nearest(const GrayImage& image, uint32_t width, uint32_t height) {
  require_shape(image);
  GrayImage out = make_image(width, height, 0);
  if (width == 0 || height == 0) return out;
  for (uint32_t y = 0; y < height; ++y) {
    uint32_t sy = std::min<uint32_t>(image.height - 1, static_cast<uint64_t>(y) * image.height / height);
    for (uint32_t x = 0; x < width; ++x) {
      uint32_t sx = std::min<uint32_t>(image.width - 1, static_cast<uint64_t>(x) * image.width / width);
      out.pixels[pixel_index(out, x, y)] = image.pixels[pixel_index(image, sx, sy)];
    }
  }
  return out;
}

GrayImage threshold_image(const GrayImage& image, uint8_t threshold, uint8_t low, uint8_t high) {
  require_shape(image);
  GrayImage out = image;
  for (uint8_t& p : out.pixels) p = p < threshold ? low : high;
  return out;
}

GrayImage box_blur(const GrayImage& image, uint32_t radius) {
  require_shape(image);
  GrayImage out = make_image(image.width, image.height, 0);
  int r = static_cast<int>(radius);
  for (uint32_t y = 0; y < image.height; ++y) {
    for (uint32_t x = 0; x < image.width; ++x) {
      uint32_t sum = 0;
      uint32_t count = 0;
      for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
          int sx = static_cast<int>(x) + dx;
          int sy = static_cast<int>(y) + dy;
          if (sx < 0 || sy < 0 || sx >= static_cast<int>(image.width) || sy >= static_cast<int>(image.height)) continue;
          sum += image.pixels[pixel_index(image, static_cast<uint32_t>(sx), static_cast<uint32_t>(sy))];
          ++count;
        }
      }
      out.pixels[pixel_index(out, x, y)] = count == 0 ? 0 : static_cast<uint8_t>(sum / count);
    }
  }
  return out;
}

GrayImage blend_images(const GrayImage& a, const GrayImage& b, double alpha) {
  require_shape(a);
  require_shape(b);
  if (a.width != b.width || a.height != b.height) throw DecodeError("cannot blend images with different sizes");
  alpha = std::max(0.0, std::min(1.0, alpha));
  GrayImage out = a;
  for (size_t i = 0; i < out.pixels.size(); ++i) {
    double v = static_cast<double>(a.pixels[i]) * (1.0 - alpha) + static_cast<double>(b.pixels[i]) * alpha;
    out.pixels[i] = static_cast<uint8_t>(std::lround(v));
  }
  return out;
}

ImageStats analyze_image(const GrayImage& image) {
  require_shape(image);
  ImageStats stats;
  stats.histogram.assign(256, 0);
  stats.min_value = 255;
  stats.max_value = 0;
  double sum = 0.0;
  for (uint8_t p : image.pixels) {
    ++stats.histogram[p];
    stats.min_value = std::min(stats.min_value, p);
    stats.max_value = std::max(stats.max_value, p);
    sum += p;
  }
  stats.mean = image.pixels.empty() ? 0.0 : sum / static_cast<double>(image.pixels.size());
  double var = 0.0;
  for (uint8_t p : image.pixels) {
    double d = p - stats.mean;
    var += d * d;
  }
  stats.variance = image.pixels.empty() ? 0.0 : var / static_cast<double>(image.pixels.size());
  return stats;
}

uint32_t image_hash(const GrayImage& image) {
  require_shape(image);
  uint32_t state = fnv1a32(image.pixels);
  state = rotate_mix(state, image.width);
  state = rotate_mix(state, image.height);
  return state;
}

}  // namespace wayvault
