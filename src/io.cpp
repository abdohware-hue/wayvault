#include "wayvault/io.h"

#include <fstream>
#include <iterator>
#include <stdexcept>

namespace wayvault {

std::vector<unsigned char> read_file(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("failed to open input file: " + path);
  }
  return std::vector<unsigned char>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

void write_file(const std::string& path, const std::vector<unsigned char>& bytes) {
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    throw std::runtime_error("failed to open output file: " + path);
  }
  out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

}  // namespace wayvault
