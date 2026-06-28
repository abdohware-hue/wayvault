#include "wayvault/annotation_layer.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <sstream>

#include "wayvault/checksum.h"

namespace wayvault {

namespace {

std::vector<std::string> split(const std::string& line, char sep) {
  std::vector<std::string> out;
  std::string cur;
  for (char c : line) {
    if (c == sep) {
      out.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  out.push_back(cur);
  return out;
}

std::map<std::string, std::string> parse_properties(const std::string& text) {
  std::map<std::string, std::string> props;
  for (const std::string& item : split(text, ';')) {
    size_t eq = item.find('=');
    if (eq == std::string::npos) continue;
    props[item.substr(0, eq)] = item.substr(eq + 1);
  }
  return props;
}

std::string props_to_text(const std::map<std::string, std::string>& props) {
  std::ostringstream out;
  bool first = true;
  for (const auto& kv : props) {
    if (!first) out << ';';
    first = false;
    out << kv.first << '=' << kv.second;
  }
  return out.str();
}

void include_point(Bounds2& bounds, Point2 point) {
  if (!bounds.valid) bounds = {point.x, point.y, point.x, point.y, true};
  else {
    bounds.min_x = std::min(bounds.min_x, point.x);
    bounds.min_y = std::min(bounds.min_y, point.y);
    bounds.max_x = std::max(bounds.max_x, point.x);
    bounds.max_y = std::max(bounds.max_y, point.y);
  }
}

}  // namespace

AnnotationLayer parse_annotation_layer(const std::string& text, const std::string& default_layer) {
  AnnotationLayer layer;
  layer.name = default_layer;
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') continue;
    if (line.rfind("layer:", 0) == 0) {
      layer.name = line.substr(6);
      continue;
    }
    auto parts = split(line, '|');
    if (parts.size() < 5) continue;
    Annotation annotation;
    annotation.id = parts[0];
    annotation.layer = parts[1].empty() ? layer.name : parts[1];
    annotation.anchor = {std::stod(parts[2]), std::stod(parts[3])};
    annotation.properties = parse_properties(parts[4]);
    if (parts.size() > 5) annotation.min_zoom = static_cast<uint32_t>(std::stoul(parts[5]));
    if (parts.size() > 6) annotation.max_zoom = static_cast<uint32_t>(std::stoul(parts[6]));
    if (annotation.min_zoom > annotation.max_zoom) std::swap(annotation.min_zoom, annotation.max_zoom);
    layer.annotations.push_back(std::move(annotation));
  }
  return layer;
}

std::string write_annotation_layer(const AnnotationLayer& layer) {
  std::ostringstream out;
  out << "layer:" << layer.name << '\n';
  for (const Annotation& annotation : layer.annotations) {
    out << annotation.id << '|' << annotation.layer << '|' << annotation.anchor.x << '|'
        << annotation.anchor.y << '|' << props_to_text(annotation.properties) << '|'
        << annotation.min_zoom << '|' << annotation.max_zoom << '\n';
  }
  return out.str();
}

std::vector<Annotation> filter_annotations(const AnnotationLayer& layer, Bounds2 bounds, uint32_t zoom) {
  std::vector<Annotation> out;
  for (const Annotation& annotation : layer.annotations) {
    if (zoom < annotation.min_zoom || zoom > annotation.max_zoom) continue;
    if (bounds.valid && !bounds_contains(bounds, annotation.anchor)) continue;
    out.push_back(annotation);
  }
  std::sort(out.begin(), out.end(), [](const Annotation& a, const Annotation& b) {
    if (a.layer != b.layer) return a.layer < b.layer;
    return a.id < b.id;
  });
  return out;
}

AnnotationSummary summarize_annotations(const AnnotationLayer& layer) {
  AnnotationSummary summary;
  summary.count = layer.annotations.size();
  std::set<std::string> seen;
  std::set<std::string> duplicate_once;
  for (const Annotation& annotation : layer.annotations) {
    include_point(summary.bounds, annotation.anchor);
    summary.by_layer[annotation.layer]++;
    if (!seen.insert(annotation.id).second && duplicate_once.insert(annotation.id).second) {
      summary.duplicate_ids.push_back(annotation.id);
    }
  }
  return summary;
}

void merge_annotation_layer(AnnotationLayer& target, const AnnotationLayer& source, bool replace_existing) {
  std::map<std::string, size_t> positions;
  for (size_t i = 0; i < target.annotations.size(); ++i) positions[target.annotations[i].id] = i;
  for (const Annotation& annotation : source.annotations) {
    auto it = positions.find(annotation.id);
    if (it == positions.end()) {
      positions[annotation.id] = target.annotations.size();
      target.annotations.push_back(annotation);
    } else if (replace_existing) {
      target.annotations[it->second] = annotation;
    }
  }
}

std::vector<Annotation> rank_annotations_by_distance(const AnnotationLayer& layer, Point2 focus, size_t limit) {
  std::vector<Annotation> out = layer.annotations;
  std::sort(out.begin(), out.end(), [focus](const Annotation& a, const Annotation& b) {
    double da = distance(a.anchor, focus);
    double db = distance(b.anchor, focus);
    if (da != db) return da < db;
    return a.id < b.id;
  });
  if (out.size() > limit) out.resize(limit);
  return out;
}

uint32_t annotation_layer_hash(const AnnotationLayer& layer) {
  uint32_t state = fnv1a32(reinterpret_cast<const uint8_t*>(layer.name.data()), layer.name.size());
  for (const Annotation& annotation : layer.annotations) {
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(annotation.id.data()), annotation.id.size()));
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(annotation.layer.data()), annotation.layer.size()));
    state = rotate_mix(state, static_cast<uint32_t>(std::llround(annotation.anchor.x * 16.0)));
    state = rotate_mix(state, static_cast<uint32_t>(std::llround(annotation.anchor.y * 16.0)));
    state = rotate_mix(state, annotation.min_zoom);
    state = rotate_mix(state, annotation.max_zoom);
    for (const auto& kv : annotation.properties) {
      state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(kv.first.data()), kv.first.size()));
      state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(kv.second.data()), kv.second.size()));
    }
  }
  return state;
}

}  // namespace wayvault
