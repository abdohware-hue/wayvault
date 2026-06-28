#include "wayvault/compression.h"

#include <algorithm>
#include <limits>

#include "wayvault/checksum.h"
#include "wayvault/error.h"

namespace wayvault {

uint32_t zigzag_encode(int32_t value) {
  return (static_cast<uint32_t>(value) << 1) ^ static_cast<uint32_t>(value >> 31);
}

int32_t zigzag_decode(uint32_t value) {
  return static_cast<int32_t>((value >> 1) ^ (~(value & 1u) + 1u));
}

std::vector<uint8_t> encode_varints(const std::vector<uint32_t>& values) {
  std::vector<uint8_t> out;
  for (uint32_t value : values) {
    while (value >= 0x80) {
      out.push_back(static_cast<uint8_t>(value | 0x80u));
      value >>= 7;
    }
    out.push_back(static_cast<uint8_t>(value));
  }
  return out;
}

std::vector<uint32_t> decode_varints(const uint8_t* data, size_t size, size_t max_values) {
  std::vector<uint32_t> out;
  uint32_t value = 0;
  uint32_t shift = 0;
  for (size_t i = 0; i < size; ++i) {
    uint8_t byte = data[i];
    if (shift >= 32 && (byte & 0x7fu) != 0) {
      throw ParseError("varint exceeds 32 bits");
    }
    value |= static_cast<uint32_t>(byte & 0x7fu) << shift;
    if ((byte & 0x80u) == 0) {
      out.push_back(value);
      value = 0;
      shift = 0;
      if (max_values != 0 && out.size() >= max_values) return out;
    } else {
      shift += 7;
    }
  }
  if (shift != 0) throw ParseError("truncated varint sequence");
  return out;
}

std::vector<RleSpan> rle_encode(const std::vector<uint8_t>& bytes, uint16_t max_run) {
  std::vector<RleSpan> spans;
  if (bytes.empty()) return spans;
  if (max_run == 0) max_run = std::numeric_limits<uint16_t>::max();
  uint8_t value = bytes.front();
  uint16_t run = 0;
  for (uint8_t byte : bytes) {
    if (byte == value && run < max_run) {
      ++run;
    } else {
      spans.push_back(RleSpan{run, value});
      value = byte;
      run = 1;
    }
  }
  spans.push_back(RleSpan{run, value});
  return spans;
}

std::vector<uint8_t> rle_decode_spans(const std::vector<RleSpan>& spans, size_t expected_limit) {
  std::vector<uint8_t> out;
  for (const RleSpan& span : spans) {
    if (span.run == 0) continue;
    if (expected_limit != 0 && out.size() + span.run > expected_limit) {
      throw DecodeError("rle span exceeds expected limit");
    }
    out.insert(out.end(), span.run, span.value);
  }
  return out;
}

DeltaSeries delta_encode(const std::vector<int32_t>& values) {
  DeltaSeries series;
  if (values.empty()) return series;
  series.base = values.front();
  series.deltas.reserve(values.size() - 1);
  int32_t prev = values.front();
  for (size_t i = 1; i < values.size(); ++i) {
    series.deltas.push_back(values[i] - prev);
    prev = values[i];
  }
  return series;
}

std::vector<int32_t> delta_decode(const DeltaSeries& series, size_t limit) {
  std::vector<int32_t> out;
  if (limit == 0) limit = series.deltas.size() + 1;
  out.reserve(std::min(limit, series.deltas.size() + 1));
  int32_t cur = series.base;
  out.push_back(cur);
  for (int32_t delta : series.deltas) {
    if (out.size() >= limit) break;
    cur += delta;
    out.push_back(cur);
  }
  return out;
}

std::vector<uint8_t> pack_nibbles(const std::vector<uint8_t>& values) {
  std::vector<uint8_t> out;
  out.reserve((values.size() + 1) / 2);
  for (size_t i = 0; i < values.size(); i += 2) {
    uint8_t lo = values[i] & 0x0fu;
    uint8_t hi = (i + 1 < values.size()) ? (values[i + 1] & 0x0fu) : 0;
    out.push_back(static_cast<uint8_t>(lo | (hi << 4)));
  }
  return out;
}

std::vector<uint8_t> unpack_nibbles(const std::vector<uint8_t>& bytes, size_t value_count) {
  std::vector<uint8_t> out;
  out.reserve(value_count == 0 ? bytes.size() * 2 : value_count);
  for (uint8_t byte : bytes) {
    if (value_count != 0 && out.size() >= value_count) break;
    out.push_back(byte & 0x0fu);
    if (value_count != 0 && out.size() >= value_count) break;
    out.push_back((byte >> 4) & 0x0fu);
  }
  return out;
}

uint32_t compression_fingerprint(const std::vector<uint8_t>& bytes) {
  uint32_t state = fnv1a32(bytes);
  auto spans = rle_encode(bytes, 255);
  state = rotate_mix(state, static_cast<uint32_t>(spans.size()));
  std::vector<uint32_t> values;
  for (const RleSpan& span : spans) {
    values.push_back((static_cast<uint32_t>(span.run) << 8) | span.value);
  }
  std::vector<uint8_t> encoded = encode_varints(values);
  state = rotate_mix(state, fnv1a32(encoded));
  return state;
}

}  // namespace wayvault
