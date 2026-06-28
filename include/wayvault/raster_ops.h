#pragma once

#include <cstdint>
#include <vector>

namespace wayvault {

struct GrayImage {
  uint32_t width = 0;
  uint32_t height = 0;
  std::vector<uint8_t> pixels;
};

struct ImageStats {
  uint8_t min_value = 0;
  uint8_t max_value = 0;
  double mean = 0.0;
  double variance = 0.0;
  std::vector<uint32_t> histogram;
};

GrayImage make_image(uint32_t width, uint32_t height, uint8_t fill);
uint8_t sample_nearest(const GrayImage& image, int x, int y, uint8_t outside);
GrayImage resize_nearest(const GrayImage& image, uint32_t width, uint32_t height);
GrayImage threshold_image(const GrayImage& image, uint8_t threshold, uint8_t low, uint8_t high);
GrayImage box_blur(const GrayImage& image, uint32_t radius);
GrayImage blend_images(const GrayImage& a, const GrayImage& b, double alpha);
ImageStats analyze_image(const GrayImage& image);
uint32_t image_hash(const GrayImage& image);

}  // namespace wayvault
