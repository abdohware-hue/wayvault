#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace wayvault {

struct LayerConfig {
  std::string name;
  int order = 0;
  double opacity = 1.0;
  bool visible = true;
  std::string blend_mode = "normal";
};

struct LayerStack {
  std::vector<LayerConfig> layers;
};

LayerStack parse_layer_stack(const std::string& text);
std::vector<LayerConfig> ordered_visible_layers(const LayerStack& stack);
LayerStack merge_layer_stacks(const LayerStack& base, const LayerStack& overlay);
std::string render_layer_stack(const LayerStack& stack);
uint32_t layer_stack_hash(const LayerStack& stack);

}  // namespace wayvault
