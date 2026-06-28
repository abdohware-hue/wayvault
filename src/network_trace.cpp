#include "wayvault/network_trace.h"

#include <algorithm>
#include <sstream>

#include "wayvault/checksum.h"

namespace wayvault {

namespace {

std::vector<std::string> split_csv_line(const std::string& line) {
  std::vector<std::string> out;
  std::string cur;
  bool quote = false;
  for (char c : line) {
    if (c == '"') quote = !quote;
    else if (c == ',' && !quote) {
      out.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  out.push_back(cur);
  return out;
}

}  // namespace

std::vector<TraceEvent> parse_trace_log(const std::string& text) {
  std::vector<TraceEvent> events;
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') continue;
    auto cells = split_csv_line(line);
    if (cells.size() < 5) continue;
    TraceEvent event;
    event.time_ms = std::stoull(cells[0]);
    event.operation = cells[1];
    event.key = cells[2];
    event.bytes = static_cast<uint32_t>(std::stoul(cells[3]));
    event.status = static_cast<uint32_t>(std::stoul(cells[4]));
    events.push_back(std::move(event));
  }
  std::stable_sort(events.begin(), events.end(), [](const TraceEvent& a, const TraceEvent& b) {
    return a.time_ms < b.time_ms;
  });
  return events;
}

std::string write_trace_log(const std::vector<TraceEvent>& events) {
  std::ostringstream out;
  for (const TraceEvent& event : events) {
    out << event.time_ms << ',' << event.operation << ',' << event.key << ',' << event.bytes << ',' << event.status << '\n';
  }
  return out.str();
}

TraceSummary summarize_trace(const std::vector<TraceEvent>& events) {
  TraceSummary summary;
  summary.event_count = events.size();
  if (!events.empty()) {
    summary.first_time_ms = events.front().time_ms;
    summary.last_time_ms = events.front().time_ms;
  }
  for (const TraceEvent& event : events) {
    summary.first_time_ms = std::min(summary.first_time_ms, event.time_ms);
    summary.last_time_ms = std::max(summary.last_time_ms, event.time_ms);
    summary.transferred_bytes += event.bytes;
    summary.operations[event.operation]++;
    summary.statuses[event.status]++;
  }
  return summary;
}

std::vector<TraceEvent> trace_window(const std::vector<TraceEvent>& events, uint64_t start_ms, uint64_t end_ms) {
  std::vector<TraceEvent> out;
  if (end_ms < start_ms) std::swap(start_ms, end_ms);
  for (const TraceEvent& event : events) {
    if (event.time_ms >= start_ms && event.time_ms <= end_ms) out.push_back(event);
  }
  return out;
}

std::vector<TraceEvent> slow_trace_events(const std::vector<TraceEvent>& events, uint64_t gap_ms) {
  std::vector<TraceEvent> out;
  if (events.size() < 2) return out;
  for (size_t i = 1; i < events.size(); ++i) {
    if (events[i].time_ms >= events[i - 1].time_ms && events[i].time_ms - events[i - 1].time_ms >= gap_ms) {
      out.push_back(events[i]);
    }
  }
  return out;
}

uint32_t trace_hash(const std::vector<TraceEvent>& events) {
  uint32_t state = rotate_mix(0xd271cb43u, static_cast<uint32_t>(events.size()));
  for (const TraceEvent& event : events) {
    state = rotate_mix(state, static_cast<uint32_t>(event.time_ms));
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(event.operation.data()), event.operation.size()));
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(event.key.data()), event.key.size()));
    state = rotate_mix(state, event.bytes);
    state = rotate_mix(state, event.status);
  }
  return state;
}

}  // namespace wayvault
