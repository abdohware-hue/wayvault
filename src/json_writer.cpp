#include "wayvault/json_writer.h"

#include <iomanip>
#include <sstream>

#include "wayvault/error.h"

namespace wayvault {

JsonWriter::JsonWriter() = default;

std::string json_escape(const std::string& value) {
  std::ostringstream out;
  for (unsigned char c : value) {
    switch (c) {
      case '"': out << "\\\""; break;
      case '\\': out << "\\\\"; break;
      case '\b': out << "\\b"; break;
      case '\f': out << "\\f"; break;
      case '\n': out << "\\n"; break;
      case '\r': out << "\\r"; break;
      case '\t': out << "\\t"; break;
      default:
        if (c < 0x20) {
          out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
        } else {
          out << static_cast<char>(c);
        }
        break;
    }
  }
  return out.str();
}

void JsonWriter::before_value() {
  if (stack_.empty()) {
    if (!out_.empty()) {
      throw ParseError("json writer already has a root value");
    }
    return;
  }
  Frame& frame = stack_.back();
  if (frame.type == 'o') {
    if (!frame.after_key) {
      throw ParseError("json object value must follow a key");
    }
    frame.after_key = false;
    return;
  }
  if (!frame.first) {
    out_ += ',';
  }
  newline();
  frame.first = false;
}

void JsonWriter::newline() {
  out_ += '\n';
  for (size_t i = 0; i < stack_.size(); ++i) {
    out_ += "  ";
  }
}

void JsonWriter::begin_object() {
  before_value();
  out_ += '{';
  stack_.push_back(Frame{'o', true, false});
}

void JsonWriter::end_object() {
  if (stack_.empty() || stack_.back().type != 'o') {
    throw ParseError("json writer object stack mismatch");
  }
  bool had_values = !stack_.back().first;
  stack_.pop_back();
  if (had_values) {
    newline();
  }
  out_ += '}';
}

void JsonWriter::begin_array() {
  before_value();
  out_ += '[';
  stack_.push_back(Frame{'a', true, false});
}

void JsonWriter::end_array() {
  if (stack_.empty() || stack_.back().type != 'a') {
    throw ParseError("json writer array stack mismatch");
  }
  bool had_values = !stack_.back().first;
  stack_.pop_back();
  if (had_values) {
    newline();
  }
  out_ += ']';
}

void JsonWriter::key(const std::string& name) {
  if (stack_.empty() || stack_.back().type != 'o') {
    throw ParseError("json key outside object");
  }
  Frame& frame = stack_.back();
  if (frame.after_key) {
    throw ParseError("json key cannot follow key without value");
  }
  if (!frame.first) {
    out_ += ',';
  }
  newline();
  frame.first = false;
  out_ += '"';
  out_ += json_escape(name);
  out_ += "\": ";
  frame.after_key = true;
}

void JsonWriter::string_value(const std::string& value) {
  before_value();
  out_ += '"';
  out_ += json_escape(value);
  out_ += '"';
}

void JsonWriter::number_value(double value) {
  before_value();
  std::ostringstream temp;
  temp << std::setprecision(12) << value;
  out_ += temp.str();
}

void JsonWriter::int_value(long long value) {
  before_value();
  out_ += std::to_string(value);
}

void JsonWriter::bool_value(bool value) {
  before_value();
  out_ += value ? "true" : "false";
}

void JsonWriter::null_value() {
  before_value();
  out_ += "null";
}

std::string JsonWriter::str() const {
  if (!stack_.empty()) {
    throw ParseError("json writer has unclosed containers");
  }
  return out_;
}

std::string map_to_json_object(const std::map<std::string, std::string>& values) {
  JsonWriter writer;
  writer.begin_object();
  for (const auto& kv : values) {
    writer.key(kv.first);
    writer.string_value(kv.second);
  }
  writer.end_object();
  return writer.str();
}

std::string string_array_to_json(const std::vector<std::string>& values) {
  JsonWriter writer;
  writer.begin_array();
  for (const std::string& value : values) {
    writer.string_value(value);
  }
  writer.end_array();
  return writer.str();
}

}  // namespace wayvault
