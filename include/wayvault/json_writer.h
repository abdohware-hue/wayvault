#pragma once

#include <map>
#include <string>
#include <vector>

namespace wayvault {

class JsonWriter {
 public:
  JsonWriter();
  void begin_object();
  void end_object();
  void begin_array();
  void end_array();
  void key(const std::string& name);
  void string_value(const std::string& value);
  void number_value(double value);
  void int_value(long long value);
  void bool_value(bool value);
  void null_value();
  std::string str() const;

 private:
  struct Frame {
    char type = 0;
    bool first = true;
    bool after_key = false;
  };
  void before_value();
  void newline();
  std::string out_;
  std::vector<Frame> stack_;
};

std::string json_escape(const std::string& value);
std::string map_to_json_object(const std::map<std::string, std::string>& values);
std::string string_array_to_json(const std::vector<std::string>& values);

}  // namespace wayvault
