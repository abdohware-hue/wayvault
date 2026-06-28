#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace wayvault {

struct QuotaLimit {
  std::string group;
  uint64_t max_bytes = 0;
  uint32_t max_items = 0;
  uint32_t weight = 1;
};

struct QuotaUsage {
  std::string group;
  std::string key;
  uint64_t bytes = 0;
  uint32_t items = 0;
};

struct QuotaDecision {
  QuotaUsage usage;
  bool accepted = true;
  std::string reason;
};

std::vector<QuotaLimit> parse_quota_limits(const std::string& text);
std::vector<QuotaUsage> parse_quota_usage(const std::string& text);
std::vector<QuotaDecision> evaluate_quota(const std::vector<QuotaUsage>& usage, const std::vector<QuotaLimit>& limits);
std::map<std::string, uint64_t> quota_bytes_by_group(const std::vector<QuotaUsage>& usage);
std::string summarize_quota(const std::vector<QuotaDecision>& decisions);
uint32_t quota_hash(const std::vector<QuotaDecision>& decisions);

}  // namespace wayvault
