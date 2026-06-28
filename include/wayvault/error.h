#pragma once

#include <stdexcept>
#include <string>

namespace wayvault {

class ParseError : public std::runtime_error {
 public:
  explicit ParseError(const std::string& message) : std::runtime_error(message) {}
};

class DecodeError : public ParseError {
 public:
  explicit DecodeError(const std::string& message) : ParseError(message) {}
};

}  // namespace wayvault
