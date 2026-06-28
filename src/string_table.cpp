#include "wayvault/string_table.h"

#include <algorithm>
#include <cctype>
#include <set>

#include "wayvault/checksum.h"

namespace wayvault {

std::string normalize_label(std::string label) {
  for (char& ch : label) {
    unsigned char c = static_cast<unsigned char>(ch);
    if (std::isspace(c)) {
      ch = '-';
    } else {
      ch = static_cast<char>(std::tolower(c));
    }
  }
  while (label.find("--") != std::string::npos) {
    label.erase(label.find("--"), 1);
  }
  return label;
}

int find_string_index(const VaultFile& file, const std::string& label) {
  std::string needle = normalize_label(label);
  for (size_t i = 0; i < file.strings.size(); ++i) {
    if (normalize_label(file.strings[i]) == needle) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

std::vector<std::string> unique_normalized_strings(const VaultFile& file) {
  std::set<std::string> seen;
  for (const std::string& s : file.strings) {
    std::string normalized = normalize_label(s);
    if (!normalized.empty()) {
      seen.insert(normalized);
    }
  }
  return std::vector<std::string>(seen.begin(), seen.end());
}

uint32_t string_table_digest(const VaultFile& file) {
  uint32_t state = 0xabcdef01u;
  for (const std::string& value : unique_normalized_strings(file)) {
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(value.data()), value.size()));
  }
  return state;
}

}  // namespace wayvault
