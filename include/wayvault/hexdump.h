#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "wayvault/container.h"

namespace wayvault {

std::string hex_preview(const uint8_t* data, size_t size, size_t limit = 64);
std::string section_hex_preview(const VaultFile& file, uint32_t tag, size_t limit = 64);

}  // namespace wayvault
