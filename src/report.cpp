#include "wayvault/report.h"

#include <map>
#include <sstream>

#include "wayvault/index.h"
#include "wayvault/metadata.h"
#include "wayvault/stats.h"
#include "wayvault/storage_plan.h"

namespace wayvault {

namespace {

void add_line(ReportSection& section, const std::string& line) {
  section.lines.push_back(line);
}

}  // namespace

VaultReport build_vault_report(const VaultFile& file) {
  VaultReport report;
  ReportSection overview;
  overview.title = "Overview";
  add_line(overview, "version: " + std::to_string(file.version));
  add_line(overview, "sections: " + std::to_string(file.sections.size()));
  add_line(overview, "strings: " + std::to_string(file.strings.size()));
  add_line(overview, "tiles: " + std::to_string(file.tiles.size()));
  report.sections.push_back(overview);

  std::map<uint8_t, size_t> by_codec;
  std::map<uint8_t, size_t> by_layer;
  for (const TileRecord& tile : file.tiles) {
    ++by_codec[tile.codec];
    ++by_layer[tile.layer];
    if (tile.width == 0 || tile.height == 0) report.warnings.push_back("zero-sized tile " + std::to_string(tile.id));
  }

  ReportSection codecs;
  codecs.title = "Tile Codecs";
  for (const auto& kv : by_codec) add_line(codecs, std::to_string(kv.first) + ": " + std::to_string(kv.second));
  report.sections.push_back(codecs);

  ReportSection layers;
  layers.title = "Layers";
  for (const auto& kv : by_layer) add_line(layers, std::to_string(kv.first) + ": " + std::to_string(kv.second));
  report.sections.push_back(layers);

  MetadataSummary meta = summarize_metadata(file);
  ReportSection metadata;
  metadata.title = "Metadata";
  for (const auto& kv : meta.values) add_line(metadata, kv.first + "=" + kv.second);
  add_line(metadata, "stable_hash=" + std::to_string(meta.stable_hash));
  report.sections.push_back(metadata);

  StoragePlan plan = plan_storage_for_vault(file, 16);
  ReportSection storage;
  storage.title = "Storage";
  add_line(storage, "total=" + std::to_string(plan.total_size));
  add_line(storage, "slack=" + std::to_string(plan.slack_bytes));
  report.sections.push_back(storage);

  return report;
}

std::string render_report_text(const VaultReport& report) {
  std::ostringstream out;
  for (const ReportSection& section : report.sections) {
    out << section.title << "\n";
    for (const std::string& line : section.lines) out << "  " << line << "\n";
  }
  if (!report.warnings.empty()) {
    out << "Warnings\n";
    for (const std::string& warning : report.warnings) out << "  " << warning << "\n";
  }
  return out.str();
}

std::string render_report_markdown(const VaultReport& report) {
  std::ostringstream out;
  for (const ReportSection& section : report.sections) {
    out << "## " << section.title << "\n\n";
    for (const std::string& line : section.lines) out << "- " << line << "\n";
    out << "\n";
  }
  if (!report.warnings.empty()) {
    out << "## Warnings\n\n";
    for (const std::string& warning : report.warnings) out << "- " << warning << "\n";
  }
  return out.str();
}

size_t report_line_count(const VaultReport& report) {
  size_t count = 0;
  for (const ReportSection& section : report.sections) count += 1 + section.lines.size();
  count += report.warnings.empty() ? 0 : 1 + report.warnings.size();
  return count;
}

}  // namespace wayvault
