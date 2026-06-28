#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace wayvault {

struct RetentionRule {
  std::string prefix;
  uint32_t min_zoom = 0;
  uint32_t max_zoom = 22;
  uint64_t max_age_seconds = 0;
  uint64_t min_keep_bytes = 0;
  bool pinned = false;
};

struct RetentionCandidate {
  std::string key;
  uint32_t zoom = 0;
  uint64_t size = 0;
  uint64_t age_seconds = 0;
  bool referenced = false;
};

struct RetentionDecision {
  RetentionCandidate candidate;
  bool keep = false;
  std::string reason;
};

std::vector<RetentionRule> parse_retention_rules(const std::string& text);
RetentionDecision evaluate_retention(const RetentionCandidate& candidate, const std::vector<RetentionRule>& rules);
std::vector<RetentionDecision> plan_retention(const std::vector<RetentionCandidate>& candidates, const std::vector<RetentionRule>& rules, uint64_t target_free_bytes);
uint64_t retained_bytes(const std::vector<RetentionDecision>& decisions);
uint64_t releasable_bytes(const std::vector<RetentionDecision>& decisions);
std::string summarize_retention(const std::vector<RetentionDecision>& decisions);
uint32_t retention_hash(const std::vector<RetentionDecision>& decisions);

}  // namespace wayvault
