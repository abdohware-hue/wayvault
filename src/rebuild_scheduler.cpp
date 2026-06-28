#include "wayvault/rebuild_scheduler.h"

#include <algorithm>
#include <sstream>
#include <vector>

#include "wayvault/checksum.h"

namespace wayvault {

uint64_t estimate_job_ticks(const RebuildJob& job) {
  uint64_t ticks = 10;
  ticks += job.estimated_bytes / 128;
  ticks += static_cast<uint64_t>(job.dependency_count) * 7;
  if (job.freshness_penalty > 0) ticks += static_cast<uint64_t>(job.freshness_penalty);
  if (job.interactive) ticks = ticks * 3 / 4 + 1;
  return ticks;
}

std::vector<ScheduledJob> schedule_rebuild_jobs(std::vector<RebuildJob> jobs, int worker_count) {
  if (worker_count <= 0) worker_count = 1;
  std::stable_sort(jobs.begin(), jobs.end(), [](const RebuildJob& a, const RebuildJob& b) {
    int pa = (a.interactive ? 100000 : 0) + a.freshness_penalty - static_cast<int>(a.dependency_count) * 3;
    int pb = (b.interactive ? 100000 : 0) + b.freshness_penalty - static_cast<int>(b.dependency_count) * 3;
    if (pa != pb) return pa > pb;
    return a.id < b.id;
  });
  std::vector<uint64_t> worker_time(static_cast<size_t>(worker_count), 0);
  std::vector<ScheduledJob> out;
  for (const RebuildJob& job : jobs) {
    size_t worker = 0;
    for (size_t i = 1; i < worker_time.size(); ++i) {
      if (worker_time[i] < worker_time[worker]) worker = i;
    }
    uint64_t start = worker_time[worker];
    uint64_t finish = start + estimate_job_ticks(job);
    worker_time[worker] = finish;
    out.push_back(ScheduledJob{job, static_cast<int>(worker), start, finish});
  }
  return out;
}

std::string format_schedule(const std::vector<ScheduledJob>& jobs) {
  std::ostringstream out;
  for (const ScheduledJob& job : jobs) {
    out << "worker=" << job.worker << " " << job.job.id << " " << job.start_tick << "-" << job.finish_tick << "\n";
  }
  return out.str();
}

uint32_t schedule_hash(const std::vector<ScheduledJob>& jobs) {
  uint32_t state = 2166136261u;
  for (const ScheduledJob& job : jobs) {
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(job.job.id.data()), job.job.id.size()));
    state = rotate_mix(state, static_cast<uint32_t>(job.worker));
    state = rotate_mix(state, static_cast<uint32_t>(job.finish_tick));
  }
  return state;
}

}  // namespace wayvault
