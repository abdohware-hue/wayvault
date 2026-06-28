#include "wayvault/manifest_store.h"

#include <sstream>

namespace wayvault {

bool ManifestStore::put(ManifestRecord record) {
  bool inserted = !records_.count(record.name);
  records_[record.name] = std::move(record);
  return inserted;
}

bool ManifestStore::erase(const std::string& name) {
  return records_.erase(name) != 0;
}

const ManifestRecord* ManifestStore::get(const std::string& name) const {
  auto it = records_.find(name);
  return it == records_.end() ? nullptr : &it->second;
}

std::vector<ManifestRecord> ManifestStore::list() const {
  std::vector<ManifestRecord> out;
  for (const auto& kv : records_) out.push_back(kv.second);
  return out;
}

std::vector<ManifestRecord> ManifestStore::find_by_attribute(const std::string& key, const std::string& value) const {
  std::vector<ManifestRecord> out;
  for (const auto& kv : records_) {
    auto it = kv.second.attributes.find(key);
    if (it != kv.second.attributes.end() && it->second == value) out.push_back(kv.second);
  }
  return out;
}

ManifestStore parse_manifest_store(const std::string& text) {
  ManifestStore store;
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') continue;
    std::stringstream ss(line);
    ManifestRecord record;
    std::getline(ss, record.name, ';');
    std::string attr;
    while (std::getline(ss, attr, ';')) {
      size_t eq = attr.find('=');
      if (eq != std::string::npos) record.attributes[attr.substr(0, eq)] = attr.substr(eq + 1);
    }
    if (!record.name.empty()) store.put(record);
  }
  return store;
}

std::string serialize_manifest_store(const ManifestStore& store) {
  std::ostringstream out;
  for (const ManifestRecord& record : store.list()) {
    out << record.name;
    for (const auto& kv : record.attributes) out << ';' << kv.first << '=' << kv.second;
    out << '\n';
  }
  return out.str();
}

std::string manifest_store_summary(const ManifestStore& store) {
  std::ostringstream out;
  std::vector<ManifestRecord> records = store.list();
  out << "records=" << records.size();
  size_t attrs = 0;
  for (const ManifestRecord& record : records) attrs += record.attributes.size();
  out << " attributes=" << attrs;
  return out.str();
}

}  // namespace wayvault
