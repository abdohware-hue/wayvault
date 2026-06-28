#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace wayvault {

uint32_t fnv1a32(const uint8_t* data, size_t size);
uint32_t fnv1a32(const std::vector<uint8_t>& bytes);
uint32_t rotate_mix(uint32_t state, uint32_t value);

}  // namespace wayvault
