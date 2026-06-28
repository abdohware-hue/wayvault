#include "wayvault/event_log.h"

#include <algorithm>
#include <sstream>

#include "wayvault/checksum.h"
#include "wayvault/error.h"

namespace wayvault {

EventRecord parse_event_record(const std::string& line) {
  std::stringstream ss(line);
  std::string tick;
  EventRecord event;
  if (!std::getline(ss, tick, '|')) throw ParseError("event missing tick");
  if (!std::getline(ss, event.category, '|')) throw ParseError("event missing category");
  if (!std::getline(ss, event.message, '|')) throw ParseError("event missing message");
  event.tick = std::stoull(tick);
  std::string field;
  while (std::getline(ss, field, '|')) {
    size_t eq = field.find('=');
    if (eq != std::string::npos) event.fields[field.substr(0, eq)] = field.substr(eq + 1);
  }
  return event;
}

std::vector<EventRecord> parse_event_log(const std::string& text) {
  std::vector<EventRecord> events;
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    if (!line.empty() && line[0] != '#') events.push_back(parse_event_record(line));
  }
  std::stable_sort(events.begin(), events.end(), [](const EventRecord& a, const EventRecord& b) {
    return a.tick < b.tick;
  });
  return events;
}

EventRollup rollup_events(const std::vector<EventRecord>& events) {
  EventRollup rollup;
  rollup.count = events.size();
  if (!events.empty()) {
    rollup.first_tick = events.front().tick;
    rollup.last_tick = events.front().tick;
  }
  for (const EventRecord& event : events) {
    ++rollup.by_category[event.category];
    rollup.first_tick = std::min(rollup.first_tick, event.tick);
    rollup.last_tick = std::max(rollup.last_tick, event.tick);
  }
  return rollup;
}

std::vector<EventRecord> events_in_range(const std::vector<EventRecord>& events, uint64_t start, uint64_t end) {
  std::vector<EventRecord> out;
  for (const EventRecord& event : events) {
    if (event.tick >= start && event.tick <= end) out.push_back(event);
  }
  return out;
}

std::string format_event_rollup(const EventRollup& rollup) {
  std::ostringstream out;
  out << "count=" << rollup.count << " range=" << rollup.first_tick << "-" << rollup.last_tick;
  for (const auto& kv : rollup.by_category) out << "\n  " << kv.first << '=' << kv.second;
  return out.str();
}

uint32_t event_log_hash(const std::vector<EventRecord>& events) {
  uint32_t state = 2166136261u;
  for (const EventRecord& event : events) {
    state = rotate_mix(state, static_cast<uint32_t>(event.tick));
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(event.category.data()), event.category.size()));
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(event.message.data()), event.message.size()));
  }
  return state;
}

}  // namespace wayvault
