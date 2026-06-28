#include "wayvault/tags.h"

namespace wayvault {

std::string tag_to_string(uint32_t tag) {
  std::string out(4, '\0');
  out[0] = static_cast<char>(tag & 0xff);
  out[1] = static_cast<char>((tag >> 8) & 0xff);
  out[2] = static_cast<char>((tag >> 16) & 0xff);
  out[3] = static_cast<char>((tag >> 24) & 0xff);
  for (char& ch : out) {
    if (ch < 32 || ch > 126) {
      ch = '?';
    }
  }
  return out;
}

}  // namespace wayvault
