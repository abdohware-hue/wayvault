#pragma once

#include <map>
#include <string>
#include <vector>

namespace wayvault {

struct ManifestRecord {
  std::string name;
  std::map<std::string, std::string> attributes;
};

class ManifestStore {
 public:
  bool put(ManifestRecord record);
  bool erase(const std::string& name);
  const ManifestRecord* get(const std::string& name) const;
  std::vector<ManifestRecord> list() const;
  std::vector<ManifestRecord> find_by_attribute(const std::string& key, const std::string& value) const;

 private:
  std::map<std::string, ManifestRecord> records_;
};

ManifestStore parse_manifest_store(const std::string& text);
std::string serialize_manifest_store(const ManifestStore& store);
std::string manifest_store_summary(const ManifestStore& store);

}  // namespace wayvault
