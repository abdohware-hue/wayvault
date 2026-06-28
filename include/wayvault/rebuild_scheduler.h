#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace wayvault {

struct RebuildJob {
  std::string id;
  uint8_t layer = 0;
  uint32_t estimated_bytes = 0;
  uint32_t dependency_count = 0;
  int freshness_penalty = 0;
  bool interactive = false;
};

struct ScheduledJob {
  RebuildJob job;
  int worker = 0;
  uint64_t start_tick = 0;
  uint64_t finish_tick = 0;
};

std::vector<ScheduledJob> schedule_rebuild_jobs(std::vector<RebuildJob> jobs, int worker_count);
uint64_t estimate_job_ticks(const RebuildJob& job);
std::string format_schedule(const std::vector<ScheduledJob>& jobs);
uint32_t schedule_hash(const std::vector<ScheduledJob>& jobs);

}  // namespace wayvault
