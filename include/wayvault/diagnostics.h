#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace wayvault {

enum class Severity {
  kInfo,
  kWarning,
  kError,
};

struct Diagnostic {
  Severity severity = Severity::kInfo;
  std::string code;
  std::string message;
  uint32_t related_id = 0;
};

class DiagnosticSet {
 public:
  void add(Severity severity, std::string code, std::string message, uint32_t related_id = 0);
  void info(std::string code, std::string message, uint32_t related_id = 0);
  void warning(std::string code, std::string message, uint32_t related_id = 0);
  void error(std::string code, std::string message, uint32_t related_id = 0);

  bool empty() const { return diagnostics_.empty(); }
  bool has_errors() const;
  size_t count(Severity severity) const;
  const std::vector<Diagnostic>& entries() const { return diagnostics_; }
  std::string report() const;

 private:
  std::vector<Diagnostic> diagnostics_;
};

std::string severity_name(Severity severity);
uint32_t diagnostic_fingerprint(const DiagnosticSet& set);

}  // namespace wayvault
