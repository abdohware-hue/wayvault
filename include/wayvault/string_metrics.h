#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace wayvault {

struct StringMatch {
  std::string value;
  size_t distance = 0;
  double score = 0.0;
};

size_t levenshtein_distance(const std::string& a, const std::string& b);
double normalized_string_score(const std::string& a, const std::string& b);
std::vector<StringMatch> rank_strings(const std::vector<std::string>& values, const std::string& query, size_t limit);
std::vector<std::string> wrap_text(const std::string& text, size_t width);
std::string common_prefix(const std::vector<std::string>& values);
std::string common_suffix(const std::vector<std::string>& values);

}  // namespace wayvault
