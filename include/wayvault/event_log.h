#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace wayvault {

struct EventRecord {
  uint64_t tick = 0;
  std::string category;
  std::string message;
  std::map<std::string, std::string> fields;
};

struct EventRollup {
  std::map<std::string, size_t> by_category;
  uint64_t first_tick = 0;
  uint64_t last_tick = 0;
  size_t count = 0;
};

EventRecord parse_event_record(const std::string& line);
std::vector<EventRecord> parse_event_log(const std::string& text);
EventRollup rollup_events(const std::vector<EventRecord>& events);
std::vector<EventRecord> events_in_range(const std::vector<EventRecord>& events, uint64_t start, uint64_t end);
std::string format_event_rollup(const EventRollup& rollup);
uint32_t event_log_hash(const std::vector<EventRecord>& events);

}  // namespace wayvault
