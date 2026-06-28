#include "wayvault/diagnostics.h"

#include <sstream>

#include "wayvault/checksum.h"

namespace wayvault {

void DiagnosticSet::add(Severity severity, std::string code, std::string message, uint32_t related_id) {
  diagnostics_.push_back(Diagnostic{severity, std::move(code), std::move(message), related_id});
}

void DiagnosticSet::info(std::string code, std::string message, uint32_t related_id) {
  add(Severity::kInfo, std::move(code), std::move(message), related_id);
}

void DiagnosticSet::warning(std::string code, std::string message, uint32_t related_id) {
  add(Severity::kWarning, std::move(code), std::move(message), related_id);
}

void DiagnosticSet::error(std::string code, std::string message, uint32_t related_id) {
  add(Severity::kError, std::move(code), std::move(message), related_id);
}

bool DiagnosticSet::has_errors() const {
  for (const Diagnostic& diagnostic : diagnostics_) {
    if (diagnostic.severity == Severity::kError) {
      return true;
    }
  }
  return false;
}

size_t DiagnosticSet::count(Severity severity) const {
  size_t n = 0;
  for (const Diagnostic& diagnostic : diagnostics_) {
    if (diagnostic.severity == severity) {
      ++n;
    }
  }
  return n;
}

std::string DiagnosticSet::report() const {
  std::ostringstream out;
  for (const Diagnostic& diagnostic : diagnostics_) {
    out << severity_name(diagnostic.severity) << " " << diagnostic.code;
    if (diagnostic.related_id != 0) {
      out << " #" << diagnostic.related_id;
    }
    out << ": " << diagnostic.message << "\n";
  }
  return out.str();
}

std::string severity_name(Severity severity) {
  switch (severity) {
    case Severity::kInfo:
      return "info";
    case Severity::kWarning:
      return "warning";
    case Severity::kError:
      return "error";
  }
  return "unknown";
}

uint32_t diagnostic_fingerprint(const DiagnosticSet& set) {
  uint32_t state = 0x13572468u;
  for (const Diagnostic& diagnostic : set.entries()) {
    state = rotate_mix(state, static_cast<uint32_t>(diagnostic.severity));
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(diagnostic.code.data()), diagnostic.code.size()));
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(diagnostic.message.data()), diagnostic.message.size()));
    state = rotate_mix(state, diagnostic.related_id);
  }
  return state;
}

}  // namespace wayvault
