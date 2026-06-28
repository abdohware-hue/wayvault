#include "wayvault/journal.h"

#include <algorithm>
#include <sstream>

#include "wayvault/checksum.h"
#include "wayvault/error.h"

namespace wayvault {

namespace {

std::vector<std::string> split_fields(const std::string& line) {
  std::vector<std::string> fields;
  std::string cur;
  bool escape = false;
  for (char c : line) {
    if (escape) {
      cur.push_back(c);
      escape = false;
    } else if (c == '\\') {
      escape = true;
    } else if (c == '|') {
      fields.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  fields.push_back(cur);
  return fields;
}

}  // namespace

uint32_t journal_entry_hash(const JournalEntry& entry) {
  uint32_t state = rotate_mix(static_cast<uint32_t>(entry.sequence), static_cast<uint32_t>(entry.sequence >> 32));
  state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(entry.op.data()), entry.op.size()));
  state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(entry.key.data()), entry.key.size()));
  state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(entry.value.data()), entry.value.size()));
  return state;
}

JournalEntry parse_journal_line(const std::string& line) {
  std::vector<std::string> fields = split_fields(line);
  if (fields.size() < 4) throw ParseError("journal line needs sequence, op, key, value");
  JournalEntry entry;
  entry.sequence = std::stoull(fields[0]);
  entry.op = fields[1];
  entry.key = fields[2];
  entry.value = fields[3];
  entry.checksum = fields.size() >= 5 && !fields[4].empty() ? static_cast<uint32_t>(std::stoul(fields[4])) : 0;
  if (entry.checksum != 0 && entry.checksum != journal_entry_hash(entry)) {
    throw ParseError("journal entry checksum mismatch");
  }
  return entry;
}

std::vector<JournalEntry> parse_journal_text(const std::string& text) {
  std::vector<JournalEntry> entries;
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') continue;
    entries.push_back(parse_journal_line(line));
  }
  return entries;
}

JournalState replay_journal(const std::vector<JournalEntry>& entries, bool strict_order) {
  JournalState state;
  state.rolling_hash = 2166136261u;
  for (const JournalEntry& entry : entries) {
    if (strict_order && entry.sequence <= state.last_sequence) {
      state.warnings.push_back("sequence rewind at " + std::to_string(entry.sequence));
      continue;
    }
    state.last_sequence = std::max(state.last_sequence, entry.sequence);
    if (entry.op == "set") {
      state.values[entry.key] = entry.value;
    } else if (entry.op == "erase") {
      state.values.erase(entry.key);
    } else if (entry.op == "append") {
      state.values[entry.key] += entry.value;
    } else if (entry.op == "touch") {
      state.values.emplace(entry.key, entry.value);
    } else {
      state.warnings.push_back("unknown journal op " + entry.op);
    }
    state.rolling_hash = rotate_mix(state.rolling_hash, journal_entry_hash(entry));
  }
  return state;
}

std::string summarize_journal_state(const JournalState& state) {
  std::ostringstream out;
  out << "last=" << state.last_sequence << " values=" << state.values.size() << " hash=" << state.rolling_hash;
  for (const auto& kv : state.values) out << "\n  " << kv.first << "=" << kv.second;
  for (const std::string& warning : state.warnings) out << "\n  warning: " << warning;
  return out.str();
}

}  // namespace wayvault
