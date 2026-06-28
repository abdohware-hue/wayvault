#include "wayvault/vault_analyzer.h"

#include <algorithm>
#include <sstream>

#include "wayvault/checksum.h"

namespace wayvault {

namespace {

void append_all(std::vector<AnalyzerWarning>& target, std::vector<AnalyzerWarning> source) {
  target.insert(target.end(), std::make_move_iterator(source.begin()), std::make_move_iterator(source.end()));
}

}  // namespace

std::vector<AnalyzerWarning> check_zoom_gaps(const TileMatrix& matrix) {
  std::vector<AnalyzerWarning> warnings;
  auto summary = summarize_tile_matrix(matrix);
  if (summary.by_zoom.empty()) return warnings;
  uint32_t previous = summary.by_zoom.begin()->first;
  for (auto it = std::next(summary.by_zoom.begin()); it != summary.by_zoom.end(); ++it) {
    if (it->first > previous + 1) {
      warnings.push_back({"zoom-gap", "z" + std::to_string(previous) + "-z" + std::to_string(it->first), "missing intermediate zoom level", 3});
    }
    previous = it->first;
  }
  for (const auto& kv : summary.by_zoom) {
    if (kv.second == 1 && kv.first > 4) warnings.push_back({"thin-level", "z" + std::to_string(kv.first), "only one tile at a detailed zoom", 2});
  }
  return warnings;
}

std::vector<AnalyzerWarning> check_payload_layout(const TileMatrix& matrix) {
  std::vector<AnalyzerWarning> warnings;
  std::vector<MatrixTileRecord> records;
  for (const TileMatrixLevel& level : matrix.levels()) records.insert(records.end(), level.records.begin(), level.records.end());
  std::sort(records.begin(), records.end(), [](const MatrixTileRecord& a, const MatrixTileRecord& b) {
    return a.offset < b.offset;
  });
  uint64_t end = 0;
  for (const MatrixTileRecord& record : records) {
    if (record.size == 0) warnings.push_back({"empty-tile", std::to_string(record.coord.z) + "/" + std::to_string(record.coord.x) + "/" + std::to_string(record.coord.y), "tile has no payload", 1});
    if (record.offset < end) warnings.push_back({"overlap", std::to_string(record.offset), "payload overlaps previous tile", 5});
    if (record.offset > end + 4096) warnings.push_back({"gap", std::to_string(end), "large unused payload gap", 1});
    end = std::max(end, record.offset + record.size);
  }
  return warnings;
}

std::vector<AnalyzerWarning> check_metadata_expectations(const std::map<std::string, std::string>& metadata) {
  std::vector<AnalyzerWarning> warnings;
  for (const std::string& key : {"name", "version", "created_by"}) {
    if (!metadata.count(key)) warnings.push_back({"metadata-missing", key, "recommended metadata key is absent", 2});
  }
  auto version = metadata.find("version");
  if (version != metadata.end() && version->second.empty()) warnings.push_back({"metadata-empty", "version", "version is empty", 2});
  auto bounds = metadata.find("bounds");
  if (bounds != metadata.end() && bounds->second.find(',') == std::string::npos) warnings.push_back({"metadata-format", "bounds", "bounds should contain comma separated values", 1});
  return warnings;
}

uint32_t compute_health_score(const std::vector<AnalyzerWarning>& warnings) {
  int score = 100;
  for (const AnalyzerWarning& warning : warnings) score -= static_cast<int>(warning.severity) * 4;
  if (score < 0) score = 0;
  return static_cast<uint32_t>(score);
}

VaultAnalysis analyze_vault_matrix(const TileMatrix& matrix, const std::map<std::string, std::string>& metadata) {
  VaultAnalysis analysis;
  analysis.matrix_summary = summarize_tile_matrix(matrix);
  analysis.metadata = metadata;
  append_all(analysis.warnings, check_zoom_gaps(matrix));
  append_all(analysis.warnings, check_payload_layout(matrix));
  append_all(analysis.warnings, check_metadata_expectations(metadata));
  std::sort(analysis.warnings.begin(), analysis.warnings.end(), [](const AnalyzerWarning& a, const AnalyzerWarning& b) {
    if (a.severity != b.severity) return a.severity > b.severity;
    if (a.code != b.code) return a.code < b.code;
    return a.subject < b.subject;
  });
  analysis.health_score = compute_health_score(analysis.warnings);
  return analysis;
}

std::string write_vault_analysis(const VaultAnalysis& analysis) {
  std::ostringstream out;
  out << "health=" << analysis.health_score << '\n';
  out << "tiles=" << analysis.matrix_summary.tile_count << " bytes=" << analysis.matrix_summary.byte_count << '\n';
  for (const auto& kv : analysis.matrix_summary.by_zoom) out << "zoom " << kv.first << " tiles=" << kv.second << " bytes=" << analysis.matrix_summary.bytes_by_zoom.at(kv.first) << '\n';
  for (const AnalyzerWarning& warning : analysis.warnings) out << warning.severity << ' ' << warning.code << ' ' << warning.subject << " - " << warning.detail << '\n';
  return out.str();
}

uint32_t vault_analysis_hash(const VaultAnalysis& analysis) {
  uint32_t state = rotate_mix(0x66de2819u, analysis.health_score);
  state = rotate_mix(state, static_cast<uint32_t>(analysis.matrix_summary.tile_count));
  state = rotate_mix(state, static_cast<uint32_t>(analysis.matrix_summary.byte_count));
  for (const auto& kv : analysis.metadata) {
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(kv.first.data()), kv.first.size()));
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(kv.second.data()), kv.second.size()));
  }
  for (const AnalyzerWarning& warning : analysis.warnings) {
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(warning.code.data()), warning.code.size()));
    state = rotate_mix(state, warning.severity);
  }
  return state;
}

}  // namespace wayvault
