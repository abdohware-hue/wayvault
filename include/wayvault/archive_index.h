#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace wayvault {

struct ArchiveEntry {
  std::string path;
  uint64_t offset = 0;
  uint64_t size = 0;
  uint32_t checksum = 0;
};

class ArchiveIndex {
 public:
  bool add(ArchiveEntry entry);
  const ArchiveEntry* find(const std::string& path) const;
  std::vector<ArchiveEntry> entries_under(const std::string& prefix) const;
  uint64_t total_size() const;
  size_t size() const { return entries_.size(); }

 private:
  std::map<std::string, ArchiveEntry> entries_;
};

ArchiveIndex parse_archive_index(const std::string& text);
std::string format_archive_index(const ArchiveIndex& index);
uint32_t archive_index_hash(const ArchiveIndex& index);

}  // namespace wayvault
