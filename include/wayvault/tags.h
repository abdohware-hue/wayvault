#pragma once

#include <cstdint>
#include <string>

namespace wayvault {

constexpr uint32_t make_tag(char a, char b, char c, char d) {
  return static_cast<uint32_t>(static_cast<unsigned char>(a)) |
         (static_cast<uint32_t>(static_cast<unsigned char>(b)) << 8) |
         (static_cast<uint32_t>(static_cast<unsigned char>(c)) << 16) |
         (static_cast<uint32_t>(static_cast<unsigned char>(d)) << 24);
}

constexpr uint32_t TAG_STRS = make_tag('S', 'T', 'R', 'S');
constexpr uint32_t TAG_TILE = make_tag('T', 'I', 'L', 'E');
constexpr uint32_t TAG_DATA = make_tag('D', 'A', 'T', 'A');
constexpr uint32_t TAG_META = make_tag('M', 'E', 'T', 'A');
constexpr uint32_t TAG_CMDS = make_tag('C', 'M', 'D', 'S');
constexpr uint32_t TAG_INDX = make_tag('I', 'N', 'D', 'X');

std::string tag_to_string(uint32_t tag);

}  // namespace wayvault
