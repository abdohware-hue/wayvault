#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "wayvault/container.h"

namespace wayvault {

struct ColorRGBA {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  uint8_t a = 255;
};

struct StyleRule {
  std::string layer;
  std::string key;
  std::string value;
  ColorRGBA fill;
  ColorRGBA stroke;
  double width = 1.0;
  int priority = 0;
  bool visible = true;
};

struct ResolvedStyle {
  ColorRGBA fill;
  ColorRGBA stroke;
  double width = 1.0;
  int priority = 0;
  bool visible = true;
  std::vector<std::string> matched_layers;
};

class StyleSheet {
 public:
  void add_rule(StyleRule rule);
  const std::vector<StyleRule>& rules() const { return rules_; }
  ResolvedStyle resolve(const std::map<std::string, std::string>& tags, const std::string& layer) const;
  std::vector<StyleRule> rules_for_layer(const std::string& layer) const;
  size_t size() const { return rules_.size(); }

 private:
  std::vector<StyleRule> rules_;
};

ColorRGBA parse_color(const std::string& text);
std::string color_to_hex(ColorRGBA color, bool include_alpha);
StyleRule parse_style_rule(const std::string& line);
StyleSheet parse_style_sheet(const std::string& text);
bool rule_matches(const StyleRule& rule, const std::map<std::string, std::string>& tags, const std::string& layer);
std::string dump_style_sheet(const StyleSheet& sheet);
uint32_t style_hash(const ResolvedStyle& style);

}  // namespace wayvault
