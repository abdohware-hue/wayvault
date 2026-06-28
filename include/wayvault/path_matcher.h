#pragma once

#include <string>
#include <vector>

namespace wayvault {

struct PathPattern {
  std::vector<std::string> segments;
  bool anchored = true;
};

PathPattern compile_path_pattern(const std::string& pattern);
bool path_matches(const PathPattern& pattern, const std::string& path);
std::vector<std::string> split_path_segments(const std::string& path);
std::vector<std::string> filter_matching_paths(const std::vector<std::string>& paths, const PathPattern& pattern);
std::string normalize_path_text(const std::string& path);

}  // namespace wayvault
