#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace wayvault {

struct SampleTileSpec {
  uint32_t id = 1;
  std::string name;
  uint16_t width = 4;
  uint16_t height = 4;
  uint8_t codec = 0;
  std::vector<uint8_t> payload;
};

struct SampleCommandSpec {
  uint8_t op = 1;
  uint32_t tile_id = 1;
  uint16_t arg = 0;
  uint8_t repeat = 1;
};

std::vector<unsigned char> build_sample_vault(const std::vector<SampleTileSpec>& tiles,
                                              const std::vector<SampleCommandSpec>& commands,
                                              bool include_metadata = true);

std::vector<unsigned char> build_default_sample();

}  // namespace wayvault
