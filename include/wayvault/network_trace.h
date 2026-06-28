#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace wayvault {

struct TraceEvent {
  uint64_t time_ms = 0;
  std::string operation;
  std::string key;
  uint32_t bytes = 0;
  uint32_t status = 0;
};

struct TraceSummary {
  size_t event_count = 0;
  uint64_t first_time_ms = 0;
  uint64_t last_time_ms = 0;
  uint64_t transferred_bytes = 0;
  std::map<std::string, size_t> operations;
  std::map<uint32_t, size_t> statuses;
};

std::vector<TraceEvent> parse_trace_log(const std::string& text);
std::string write_trace_log(const std::vector<TraceEvent>& events);
TraceSummary summarize_trace(const std::vector<TraceEvent>& events);
std::vector<TraceEvent> trace_window(const std::vector<TraceEvent>& events, uint64_t start_ms, uint64_t end_ms);
std::vector<TraceEvent> slow_trace_events(const std::vector<TraceEvent>& events, uint64_t gap_ms);
uint32_t trace_hash(const std::vector<TraceEvent>& events);

}  // namespace wayvault
