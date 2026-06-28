#include "wayvault/checksum.h"

namespace wayvault {

uint32_t fnv1a32(const uint8_t* data, size_t size) {
  uint32_t hash = 2166136261u;
  for (size_t i = 0; i < size; ++i) {
    hash ^= data[i];
    hash *= 16777619u;
  }
  return hash;
}

uint32_t fnv1a32(const std::vector<uint8_t>& bytes) {
  return fnv1a32(bytes.data(), bytes.size());
}

uint32_t rotate_mix(uint32_t state, uint32_t value) {
  uint32_t rotated = (state << 5) | (state >> 27);
  return rotated ^ (value + 0x9e3779b9u + (state << 6) + (state >> 2));
}

}  // namespace wayvault
