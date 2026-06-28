#include "wayvault/string_metrics.h"

#include <algorithm>
#include <cctype>
#include <vector>

namespace wayvault {

namespace {

std::string lower_copy(const std::string& value) {
  std::string out;
  out.reserve(value.size());
  for (unsigned char c : value) {
    out.push_back(static_cast<char>(std::tolower(c)));
  }
  return out;
}

}  // namespace

size_t levenshtein_distance(const std::string& a, const std::string& b) {
  std::string left = lower_copy(a);
  std::string right = lower_copy(b);
  std::vector<size_t> prev(right.size() + 1);
  std::vector<size_t> cur(right.size() + 1);
  for (size_t j = 0; j <= right.size(); ++j) {
    prev[j] = j;
  }
  for (size_t i = 1; i <= left.size(); ++i) {
    cur[0] = i;
    for (size_t j = 1; j <= right.size(); ++j) {
      size_t cost = left[i - 1] == right[j - 1] ? 0 : 1;
      size_t del = prev[j] + 1;
      size_t ins = cur[j - 1] + 1;
      size_t sub = prev[j - 1] + cost;
      cur[j] = std::min({del, ins, sub});
    }
    std::swap(prev, cur);
  }
  return prev[right.size()];
}

double normalized_string_score(const std::string& a, const std::string& b) {
  size_t max_len = std::max(a.size(), b.size());
  if (max_len == 0) {
    return 1.0;
  }
  size_t d = levenshtein_distance(a, b);
  double edit_score = 1.0 - static_cast<double>(d) / static_cast<double>(max_len);
  std::string la = lower_copy(a);
  std::string lb = lower_copy(b);
  if (la.find(lb) != std::string::npos || lb.find(la) != std::string::npos) {
    edit_score = std::max(edit_score, 0.75);
  }
  return std::max(0.0, edit_score);
}

std::vector<StringMatch> rank_strings(const std::vector<std::string>& values, const std::string& query, size_t limit) {
  std::vector<StringMatch> matches;
  for (const std::string& value : values) {
    StringMatch match;
    match.value = value;
    match.distance = levenshtein_distance(value, query);
    match.score = normalized_string_score(value, query);
    matches.push_back(match);
  }
  std::stable_sort(matches.begin(), matches.end(), [](const StringMatch& a, const StringMatch& b) {
    if (a.score != b.score) return a.score > b.score;
    if (a.distance != b.distance) return a.distance < b.distance;
    return a.value < b.value;
  });
  if (limit != 0 && matches.size() > limit) matches.resize(limit);
  return matches;
}

std::vector<std::string> wrap_text(const std::string& text, size_t width) {
  std::vector<std::string> lines;
  if (width == 0) {
    lines.push_back(text);
    return lines;
  }
  std::string cur;
  std::string word;
  auto flush_word = [&]() {
    if (word.empty()) return;
    if (cur.empty()) {
      cur = word;
    } else if (cur.size() + 1 + word.size() <= width) {
      cur += ' ';
      cur += word;
    } else {
      lines.push_back(cur);
      cur = word;
    }
    word.clear();
  };
  for (char c : text) {
    if (std::isspace(static_cast<unsigned char>(c))) flush_word();
    else word.push_back(c);
  }
  flush_word();
  if (!cur.empty()) lines.push_back(cur);
  return lines;
}

std::string common_prefix(const std::vector<std::string>& values) {
  if (values.empty()) return {};
  std::string prefix = values.front();
  for (const std::string& value : values) {
    while (value.compare(0, prefix.size(), prefix) != 0) {
      if (prefix.empty()) return {};
      prefix.pop_back();
    }
  }
  return prefix;
}

std::string common_suffix(const std::vector<std::string>& values) {
  if (values.empty()) return {};
  std::string suffix = values.front();
  for (const std::string& value : values) {
    while (suffix.size() > value.size() || value.compare(value.size() - suffix.size(), suffix.size(), suffix) != 0) {
      if (suffix.empty()) return {};
      suffix.erase(suffix.begin());
    }
  }
  return suffix;
}

}  // namespace wayvault
