#include "wayvault/path_matcher.h"

#include <algorithm>

namespace wayvault {

std::string normalize_path_text(const std::string& path) {
  std::string out;
  bool slash = false;
  for (char c : path) {
    if (c == '/' || c == '\\') {
      if (!slash) out.push_back('/');
      slash = true;
    } else {
      out.push_back(c);
      slash = false;
    }
  }
  if (out.size() > 1 && out.back() == '/') out.pop_back();
  return out;
}

std::vector<std::string> split_path_segments(const std::string& path) {
  std::string normalized = normalize_path_text(path);
  std::vector<std::string> segments;
  size_t start = normalized.size() > 0 && normalized[0] == '/' ? 1 : 0;
  for (size_t i = start; i <= normalized.size(); ++i) {
    if (i == normalized.size() || normalized[i] == '/') {
      if (i > start) segments.push_back(normalized.substr(start, i - start));
      start = i + 1;
    }
  }
  return segments;
}

PathPattern compile_path_pattern(const std::string& pattern) {
  PathPattern compiled;
  compiled.anchored = !pattern.empty() && pattern[0] == '/';
  compiled.segments = split_path_segments(pattern);
  return compiled;
}

namespace {

bool segment_match(const std::string& pattern, const std::string& value) {
  size_t pi = 0;
  size_t vi = 0;
  size_t star = std::string::npos;
  size_t match = 0;
  while (vi < value.size()) {
    if (pi < pattern.size() && (pattern[pi] == '?' || pattern[pi] == value[vi])) {
      ++pi;
      ++vi;
    } else if (pi < pattern.size() && pattern[pi] == '*') {
      star = pi++;
      match = vi;
    } else if (star != std::string::npos) {
      pi = star + 1;
      vi = ++match;
    } else {
      return false;
    }
  }
  while (pi < pattern.size() && pattern[pi] == '*') ++pi;
  return pi == pattern.size();
}

bool match_from(const std::vector<std::string>& p, size_t pi, const std::vector<std::string>& v, size_t vi) {
  if (pi == p.size()) return vi == v.size();
  if (p[pi] == "**") {
    for (size_t skip = vi; skip <= v.size(); ++skip) {
      if (match_from(p, pi + 1, v, skip)) return true;
    }
    return false;
  }
  if (vi == v.size()) return false;
  if (!segment_match(p[pi], v[vi])) return false;
  return match_from(p, pi + 1, v, vi + 1);
}

}  // namespace

bool path_matches(const PathPattern& pattern, const std::string& path) {
  std::vector<std::string> value = split_path_segments(path);
  if (pattern.anchored) return match_from(pattern.segments, 0, value, 0);
  for (size_t start = 0; start <= value.size(); ++start) {
    std::vector<std::string> suffix(value.begin() + static_cast<std::ptrdiff_t>(start), value.end());
    if (match_from(pattern.segments, 0, suffix, 0)) return true;
  }
  return false;
}

std::vector<std::string> filter_matching_paths(const std::vector<std::string>& paths, const PathPattern& pattern) {
  std::vector<std::string> out;
  for (const std::string& path : paths) {
    if (path_matches(pattern, path)) out.push_back(path);
  }
  std::sort(out.begin(), out.end());
  return out;
}

}  // namespace wayvault
