#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "wayvault/container.h"

namespace wayvault {

std::string normalize_label(std::string label);
int find_string_index(const VaultFile& file, const std::string& label);
std::vector<std::string> unique_normalized_strings(const VaultFile& file);
uint32_t string_table_digest(const VaultFile& file);

}  // namespace wayvault
