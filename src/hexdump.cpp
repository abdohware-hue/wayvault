#include "wayvault/hexdump.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace wayvault {

std::string hex_preview(const uint8_t* data, size_t size, size_t limit) {
  size_t n = std::min(size, limit);
  std::ostringstream out;
  for (size_t i = 0; i < n; ++i) {
    if (i != 0) {
      out << ' ';
    }
    out << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(data[i]);
  }
  if (n < size) {
    out << " ...";
  }
  return out.str();
}

std::string section_hex_preview(const VaultFile& file, uint32_t tag, size_t limit) {
  const SectionInfo* section = find_section(file, tag);
  if (section == nullptr) {
    return {};
  }
  return hex_preview(section_data(file, *section), section->size, limit);
}

}  // namespace wayvault
