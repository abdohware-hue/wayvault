#pragma once

#include <string>
#include <vector>

#include "wayvault/container.h"

namespace wayvault {

struct ReportSection {
  std::string title;
  std::vector<std::string> lines;
};

struct VaultReport {
  std::vector<ReportSection> sections;
  std::vector<std::string> warnings;
};

VaultReport build_vault_report(const VaultFile& file);
std::string render_report_text(const VaultReport& report);
std::string render_report_markdown(const VaultReport& report);
size_t report_line_count(const VaultReport& report);

}  // namespace wayvault
