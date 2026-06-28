#include "wayvault/layer_stack.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include "wayvault/checksum.h"
#include "wayvault/error.h"

namespace wayvault {

namespace {

std::string trim_layer(const std::string& value) {
  size_t a = 0;
  while (a < value.size() && std::isspace(static_cast<unsigned char>(value[a]))) ++a;
  size_t b = value.size();
  while (b > a && std::isspace(static_cast<unsigned char>(value[b - 1]))) --b;
  return value.substr(a, b - a);
}

bool parse_visible(const std::string& value) {
  return value == "1" || value == "true" || value == "yes" || value == "visible";
}

}  // namespace

LayerStack parse_layer_stack(const std::string& text) {
  LayerStack stack;
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    line = trim_layer(line);
    if (line.empty() || line[0] == '#') continue;
    std::vector<std::string> fields;
    std::string field;
    std::istringstream row(line);
    while (std::getline(row, field, ',')) fields.push_back(trim_layer(field));
    if (fields.empty()) continue;
    LayerConfig layer;
    layer.name = fields[0];
    if (fields.size() > 1 && !fields[1].empty()) layer.order = std::stoi(fields[1]);
    if (fields.size() > 2 && !fields[2].empty()) layer.opacity = std::stod(fields[2]);
    if (fields.size() > 3 && !fields[3].empty()) layer.visible = parse_visible(fields[3]);
    if (fields.size() > 4 && !fields[4].empty()) layer.blend_mode = fields[4];
    if (layer.opacity < 0.0 || layer.opacity > 1.0) throw ParseError("layer opacity out of range");
    stack.layers.push_back(layer);
  }
  return stack;
}

std::vector<LayerConfig> ordered_visible_layers(const LayerStack& stack) {
  std::vector<LayerConfig> out;
  for (const LayerConfig& layer : stack.layers) {
    if (layer.visible && layer.opacity > 0.0) out.push_back(layer);
  }
  std::stable_sort(out.begin(), out.end(), [](const LayerConfig& a, const LayerConfig& b) {
    if (a.order != b.order) return a.order < b.order;
    return a.name < b.name;
  });
  return out;
}

LayerStack merge_layer_stacks(const LayerStack& base, const LayerStack& overlay) {
  LayerStack merged = base;
  for (const LayerConfig& incoming : overlay.layers) {
    auto it = std::find_if(merged.layers.begin(), merged.layers.end(), [&](const LayerConfig& layer) {
      return layer.name == incoming.name;
    });
    if (it == merged.layers.end()) {
      merged.layers.push_back(incoming);
    } else {
      *it = incoming;
    }
  }
  return merged;
}

std::string render_layer_stack(const LayerStack& stack) {
  std::ostringstream out;
  for (const LayerConfig& layer : ordered_visible_layers(stack)) {
    out << layer.order << " " << layer.name << " opacity=" << layer.opacity << " blend=" << layer.blend_mode << "\n";
  }
  return out.str();
}

uint32_t layer_stack_hash(const LayerStack& stack) {
  uint32_t state = 2166136261u;
  for (const LayerConfig& layer : ordered_visible_layers(stack)) {
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(layer.name.data()), layer.name.size()));
    state = rotate_mix(state, static_cast<uint32_t>(layer.order));
    state = rotate_mix(state, static_cast<uint32_t>(layer.opacity * 1000000.0));
  }
  return state;
}

}  // namespace wayvault
