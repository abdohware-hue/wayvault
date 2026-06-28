#include "wayvault/change_feed.h"

#include <algorithm>
#include <sstream>

#include "wayvault/checksum.h"

namespace wayvault {

namespace {

std::vector<std::string> split(const std::string& text, char sep) {
  std::vector<std::string> out;
  std::string cur;
  for (char c : text) {
    if (c == sep) {
      out.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  out.push_back(cur);
  return out;
}

std::map<std::string, std::string> parse_fields(const std::string& text) {
  std::map<std::string, std::string> fields;
  for (const std::string& part : split(text, ';')) {
    size_t eq = part.find('=');
    if (eq != std::string::npos) fields[part.substr(0, eq)] = part.substr(eq + 1);
  }
  return fields;
}

std::string write_fields(const std::map<std::string, std::string>& fields) {
  std::ostringstream out;
  bool first = true;
  for (const auto& kv : fields) {
    if (!first) out << ';';
    first = false;
    out << kv.first << '=' << kv.second;
  }
  return out.str();
}

}  // namespace

std::vector<ChangeEvent> parse_change_feed(const std::string& text) {
  std::vector<ChangeEvent> events;
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') continue;
    auto cells = split(line, '|');
    if (cells.size() < 5) continue;
    ChangeEvent event;
    event.sequence = std::stoull(cells[0]);
    event.subject = cells[1];
    event.action = cells[2];
    event.author = cells[3];
    event.fields = parse_fields(cells[4]);
    events.push_back(std::move(event));
  }
  std::stable_sort(events.begin(), events.end(), [](const ChangeEvent& a, const ChangeEvent& b) {
    return a.sequence < b.sequence;
  });
  return events;
}

std::string write_change_feed(const std::vector<ChangeEvent>& events) {
  std::ostringstream out;
  for (const ChangeEvent& event : events) {
    out << event.sequence << '|' << event.subject << '|' << event.action << '|'
        << event.author << '|' << write_fields(event.fields) << '\n';
  }
  return out.str();
}

ChangeFeedSummary summarize_change_feed(const std::vector<ChangeEvent>& events) {
  ChangeFeedSummary summary;
  summary.event_count = events.size();
  if (!events.empty()) {
    summary.first_sequence = events.front().sequence;
    summary.last_sequence = events.front().sequence;
  }
  for (const ChangeEvent& event : events) {
    summary.first_sequence = std::min(summary.first_sequence, event.sequence);
    summary.last_sequence = std::max(summary.last_sequence, event.sequence);
    summary.by_action[event.action]++;
    summary.by_author[event.author]++;
  }
  return summary;
}

std::vector<ChangeEvent> compact_change_feed(const std::vector<ChangeEvent>& events) {
  std::map<std::string, ChangeEvent> latest_by_subject;
  std::vector<ChangeEvent> deletes;
  for (const ChangeEvent& event : events) {
    if (event.action == "delete") {
      latest_by_subject.erase(event.subject);
      deletes.push_back(event);
    } else {
      latest_by_subject[event.subject] = event;
    }
  }
  std::vector<ChangeEvent> out = deletes;
  for (const auto& kv : latest_by_subject) out.push_back(kv.second);
  std::sort(out.begin(), out.end(), [](const ChangeEvent& a, const ChangeEvent& b) {
    return a.sequence < b.sequence;
  });
  return out;
}

std::map<std::string, std::string> materialize_subject_state(const std::vector<ChangeEvent>& events, const std::string& subject) {
  std::map<std::string, std::string> state;
  for (const ChangeEvent& event : events) {
    if (event.subject != subject) continue;
    if (event.action == "delete") {
      state.clear();
      continue;
    }
    for (const auto& kv : event.fields) {
      if (event.action == "unset") state.erase(kv.first);
      else state[kv.first] = kv.second;
    }
  }
  return state;
}

std::vector<ChangeEvent> filter_change_feed(const std::vector<ChangeEvent>& events, const std::string& subject_prefix, uint64_t after_sequence) {
  std::vector<ChangeEvent> out;
  for (const ChangeEvent& event : events) {
    if (event.sequence <= after_sequence) continue;
    if (!subject_prefix.empty() && event.subject.compare(0, subject_prefix.size(), subject_prefix) != 0) continue;
    out.push_back(event);
  }
  return out;
}

uint32_t change_feed_hash(const std::vector<ChangeEvent>& events) {
  uint32_t state = rotate_mix(0xabc077a1u, static_cast<uint32_t>(events.size()));
  for (const ChangeEvent& event : events) {
    state = rotate_mix(state, static_cast<uint32_t>(event.sequence));
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(event.subject.data()), event.subject.size()));
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(event.action.data()), event.action.size()));
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(event.author.data()), event.author.size()));
    for (const auto& kv : event.fields) {
      state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(kv.first.data()), kv.first.size()));
      state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(kv.second.data()), kv.second.size()));
    }
  }
  return state;
}

}  // namespace wayvault
