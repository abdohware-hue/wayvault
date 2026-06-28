#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "wayvault/tile_matrix.h"
#include "wayvault/metadata.h"

namespace wayvault {

struct AnalyzerWarning {
  std::string code;
  std::string subject;
  std::string detail;
  uint32_t severity = 0;
};

struct VaultAnalysis {
  TileMatrixSummary matrix_summary;
  std::map<std::string, std::string> metadata;
  std::vector<AnalyzerWarning> warnings;
  uint32_t health_score = 100;
};

VaultAnalysis analyze_vault_matrix(const TileMatrix& matrix, const std::map<std::string, std::string>& metadata);
std::vector<AnalyzerWarning> check_zoom_gaps(const TileMatrix& matrix);
std::vector<AnalyzerWarning> check_payload_layout(const TileMatrix& matrix);
std::vector<AnalyzerWarning> check_metadata_expectations(const std::map<std::string, std::string>& metadata);
uint32_t compute_health_score(const std::vector<AnalyzerWarning>& warnings);
std::string write_vault_analysis(const VaultAnalysis& analysis);
uint32_t vault_analysis_hash(const VaultAnalysis& analysis);

}  // namespace wayvault
