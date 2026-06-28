#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace wayvault {

struct ChangeEvent {
  uint64_t sequence = 0;
  std::string subject;
  std::string action;
  std::string author;
  std::map<std::string, std::string> fields;
};

struct ChangeFeedSummary {
  uint64_t first_sequence = 0;
  uint64_t last_sequence = 0;
  size_t event_count = 0;
  std::map<std::string, size_t> by_action;
  std::map<std::string, size_t> by_author;
};

std::vector<ChangeEvent> parse_change_feed(const std::string& text);
std::string write_change_feed(const std::vector<ChangeEvent>& events);
ChangeFeedSummary summarize_change_feed(const std::vector<ChangeEvent>& events);
std::vector<ChangeEvent> compact_change_feed(const std::vector<ChangeEvent>& events);
std::map<std::string, std::string> materialize_subject_state(const std::vector<ChangeEvent>& events, const std::string& subject);
std::vector<ChangeEvent> filter_change_feed(const std::vector<ChangeEvent>& events, const std::string& subject_prefix, uint64_t after_sequence);
uint32_t change_feed_hash(const std::vector<ChangeEvent>& events);

}  // namespace wayvault
