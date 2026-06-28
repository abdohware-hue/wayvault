#include "wayvault/archive_index.h"

#include <sstream>

#include "wayvault/checksum.h"

namespace wayvault {

bool ArchiveIndex::add(ArchiveEntry entry) {
  bool inserted = !entries_.count(entry.path);
  entries_[entry.path] = entry;
  return inserted;
}

const ArchiveEntry* ArchiveIndex::find(const std::string& path) const {
  auto it = entries_.find(path);
  return it == entries_.end() ? nullptr : &it->second;
}

std::vector<ArchiveEntry> ArchiveIndex::entries_under(const std::string& prefix) const {
  std::vector<ArchiveEntry> out;
  for (const auto& kv : entries_) {
    if (kv.first.compare(0, prefix.size(), prefix) == 0) out.push_back(kv.second);
  }
  return out;
}

uint64_t ArchiveIndex::total_size() const {
  uint64_t total = 0;
  for (const auto& kv : entries_) total += kv.second.size;
  return total;
}

ArchiveIndex parse_archive_index(const std::string& text) {
  ArchiveIndex index;
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') continue;
    std::stringstream ss(line);
    std::string offset;
    std::string size;
    std::string checksum;
    ArchiveEntry entry;
    std::getline(ss, entry.path, '|');
    std::getline(ss, offset, '|');
    std::getline(ss, size, '|');
    std::getline(ss, checksum, '|');
    entry.offset = offset.empty() ? 0 : std::stoull(offset);
    entry.size = size.empty() ? 0 : std::stoull(size);
    entry.checksum = checksum.empty() ? 0 : static_cast<uint32_t>(std::stoul(checksum));
    index.add(entry);
  }
  return index;
}

std::string format_archive_index(const ArchiveIndex& index) {
  std::ostringstream out;
  for (const ArchiveEntry& entry : index.entries_under("")) {
    out << entry.path << '|' << entry.offset << '|' << entry.size << '|' << entry.checksum << '\n';
  }
  return out.str();
}

uint32_t archive_index_hash(const ArchiveIndex& index) {
  uint32_t state = rotate_mix(static_cast<uint32_t>(index.size()), static_cast<uint32_t>(index.total_size()));
  for (const ArchiveEntry& entry : index.entries_under("")) {
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(entry.path.data()), entry.path.size()));
    state = rotate_mix(state, entry.checksum);
  }
  return state;
}

}  // namespace wayvault
