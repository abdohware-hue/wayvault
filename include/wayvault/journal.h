#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace wayvault {

struct JournalEntry {
  uint64_t sequence = 0;
  std::string op;
  std::string key;
  std::string value;
  uint32_t checksum = 0;
};

struct JournalState {
  std::map<std::string, std::string> values;
  uint64_t last_sequence = 0;
  uint32_t rolling_hash = 0;
  std::vector<std::string> warnings;
};

JournalEntry parse_journal_line(const std::string& line);
std::vector<JournalEntry> parse_journal_text(const std::string& text);
JournalState replay_journal(const std::vector<JournalEntry>& entries, bool strict_order);
std::string summarize_journal_state(const JournalState& state);
uint32_t journal_entry_hash(const JournalEntry& entry);

}  // namespace wayvault
