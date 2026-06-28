#pragma once

#include <string>
#include <vector>

namespace wayvault {

std::vector<unsigned char> read_file(const std::string& path);
void write_file(const std::string& path, const std::vector<unsigned char>& bytes);

}  // namespace wayvault
