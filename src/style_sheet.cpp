#include "wayvault/style_sheet.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

#include "wayvault/checksum.h"
#include "wayvault/error.h"

namespace wayvault {

namespace {

std::string trim(const std::string& s) {
  size_t first = 0;
  while (first < s.size() && std::isspace(static_cast<unsigned char>(s[first]))) ++first;
  size_t last = s.size();
  while (last > first && std::isspace(static_cast<unsigned char>(s[last - 1]))) --last;
  return s.substr(first, last - first);
}

std::vector<std::string> split(const std::string& s, char sep) {
  std::vector<std::string> parts;
  size_t start = 0;
  for (size_t i = 0; i <= s.size(); ++i) {
    if (i == s.size() || s[i] == sep) {
      parts.push_back(trim(s.substr(start, i - start)));
      start = i + 1;
    }
  }
  return parts;
}

int hex_digit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  throw ParseError("invalid color hex digit");
}

uint8_t parse_hex_byte(char a, char b) {
  return static_cast<uint8_t>((hex_digit(a) << 4) | hex_digit(b));
}

bool parse_bool(const std::string& value) {
  std::string v;
  for (char c : value) v.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
  return v == "1" || v == "true" || v == "yes" || v == "on";
}

ColorRGBA named_color(const std::string& text) {
  if (text == "water") return {72, 130, 180, 255};
  if (text == "land") return {238, 232, 213, 255};
  if (text == "road") return {240, 170, 80, 255};
  if (text == "label") return {40, 40, 40, 255};
  if (text == "transparent") return {0, 0, 0, 0};
  throw ParseError("unknown style color name");
}

}  // namespace

void StyleSheet::add_rule(StyleRule rule) {
  rules_.push_back(std::move(rule));
  std::stable_sort(rules_.begin(), rules_.end(), [](const StyleRule& a, const StyleRule& b) {
    return a.priority < b.priority;
  });
}

ResolvedStyle StyleSheet::resolve(const std::map<std::string, std::string>& tags, const std::string& layer) const {
  ResolvedStyle style;
  style.fill = {230, 230, 230, 255};
  style.stroke = {120, 120, 120, 255};
  for (const StyleRule& rule : rules_) {
    if (!rule_matches(rule, tags, layer)) continue;
    style.fill = rule.fill;
    style.stroke = rule.stroke;
    style.width = rule.width;
    style.priority = rule.priority;
    style.visible = rule.visible;
    style.matched_layers.push_back(rule.layer.empty() ? "*" : rule.layer);
  }
  return style;
}

std::vector<StyleRule> StyleSheet::rules_for_layer(const std::string& layer) const {
  std::vector<StyleRule> out;
  for (const StyleRule& rule : rules_) {
    if (rule.layer.empty() || rule.layer == layer || rule.layer == "*") out.push_back(rule);
  }
  return out;
}

ColorRGBA parse_color(const std::string& text) {
  std::string value = trim(text);
  if (value.empty()) throw ParseError("empty style color");
  if (value[0] != '#') return named_color(value);
  if (value.size() == 7) {
    return {parse_hex_byte(value[1], value[2]), parse_hex_byte(value[3], value[4]),
            parse_hex_byte(value[5], value[6]), 255};
  }
  if (value.size() == 9) {
    return {parse_hex_byte(value[1], value[2]), parse_hex_byte(value[3], value[4]),
            parse_hex_byte(value[5], value[6]), parse_hex_byte(value[7], value[8])};
  }
  throw ParseError("style color must be #rrggbb or #rrggbbaa");
}

std::string color_to_hex(ColorRGBA color, bool include_alpha) {
  std::ostringstream out;
  out << '#';
  out << std::hex << std::setfill('0') << std::nouppercase;
  out << std::setw(2) << static_cast<int>(color.r);
  out << std::setw(2) << static_cast<int>(color.g);
  out << std::setw(2) << static_cast<int>(color.b);
  if (include_alpha) out << std::setw(2) << static_cast<int>(color.a);
  return out.str();
}

StyleRule parse_style_rule(const std::string& line) {
  StyleRule rule;
  std::vector<std::string> parts = split(line, ';');
  for (const std::string& part : parts) {
    if (part.empty()) continue;
    size_t eq = part.find('=');
    if (eq == std::string::npos) throw ParseError("style rule missing equals sign");
    std::string key = trim(part.substr(0, eq));
    std::string value = trim(part.substr(eq + 1));
    if (key == "layer") rule.layer = value;
    else if (key == "key") rule.key = value;
    else if (key == "value") rule.value = value;
    else if (key == "fill") rule.fill = parse_color(value);
    else if (key == "stroke") rule.stroke = parse_color(value);
    else if (key == "width") rule.width = std::stod(value);
    else if (key == "priority") rule.priority = std::stoi(value);
    else if (key == "visible") rule.visible = parse_bool(value);
    else throw ParseError("unknown style property: " + key);
  }
  return rule;
}

StyleSheet parse_style_sheet(const std::string& text) {
  StyleSheet sheet;
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    line = trim(line);
    if (line.empty() || line[0] == '#') continue;
    sheet.add_rule(parse_style_rule(line));
  }
  return sheet;
}

bool rule_matches(const StyleRule& rule, const std::map<std::string, std::string>& tags, const std::string& layer) {
  if (!rule.layer.empty() && rule.layer != "*" && rule.layer != layer) return false;
  if (!rule.key.empty()) {
    auto it = tags.find(rule.key);
    if (it == tags.end()) return false;
    if (!rule.value.empty() && rule.value != "*" && it->second != rule.value) return false;
  }
  return true;
}

std::string dump_style_sheet(const StyleSheet& sheet) {
  std::ostringstream out;
  for (const StyleRule& rule : sheet.rules()) {
    out << "layer=" << (rule.layer.empty() ? "*" : rule.layer)
        << "; key=" << rule.key
        << "; value=" << rule.value
        << "; fill=" << color_to_hex(rule.fill, rule.fill.a != 255)
        << "; stroke=" << color_to_hex(rule.stroke, rule.stroke.a != 255)
        << "; width=" << rule.width
        << "; priority=" << rule.priority
        << "; visible=" << (rule.visible ? "true" : "false") << "\n";
  }
  return out.str();
}

uint32_t style_hash(const ResolvedStyle& style) {
  uint32_t state = 2166136261u;
  state = rotate_mix(state, style.fill.r | (style.fill.g << 8) | (style.fill.b << 16) | (style.fill.a << 24));
  state = rotate_mix(state, style.stroke.r | (style.stroke.g << 8) | (style.stroke.b << 16) | (style.stroke.a << 24));
  state = rotate_mix(state, static_cast<uint32_t>(style.width * 1000.0));
  state = rotate_mix(state, static_cast<uint32_t>(style.priority));
  for (const std::string& layer : style.matched_layers) {
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(layer.data()), layer.size()));
  }
  return state;
}

}  // namespace wayvault
