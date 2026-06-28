#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace wayvault {

struct RleSpan {
  uint16_t run = 0;
  uint8_t value = 0;
};

struct DeltaSeries {
  int32_t base = 0;
  std::vector<int32_t> deltas;
};

uint32_t zigzag_encode(int32_t value);
int32_t zigzag_decode(uint32_t value);
std::vector<uint8_t> encode_varints(const std::vector<uint32_t>& values);
std::vector<uint32_t> decode_varints(const uint8_t* data, size_t size, size_t max_values);
std::vector<RleSpan> rle_encode(const std::vector<uint8_t>& bytes, uint16_t max_run);
std::vector<uint8_t> rle_decode_spans(const std::vector<RleSpan>& spans, size_t expected_limit);
DeltaSeries delta_encode(const std::vector<int32_t>& values);
std::vector<int32_t> delta_decode(const DeltaSeries& series, size_t limit);
std::vector<uint8_t> pack_nibbles(const std::vector<uint8_t>& values);
std::vector<uint8_t> unpack_nibbles(const std::vector<uint8_t>& bytes, size_t value_count);
uint32_t compression_fingerprint(const std::vector<uint8_t>& bytes);

}  // namespace wayvault
