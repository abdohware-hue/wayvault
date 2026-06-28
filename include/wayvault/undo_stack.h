#pragma once

#include <map>
#include <string>
#include <vector>

namespace wayvault {

struct StatePatch {
  std::string key;
  std::string before;
  std::string after;
};

struct UndoBatch {
  std::string label;
  std::vector<StatePatch> patches;
};

class UndoStack {
 public:
  void apply(std::map<std::string, std::string>& state, UndoBatch batch);
  bool undo(std::map<std::string, std::string>& state);
  bool redo(std::map<std::string, std::string>& state);
  void clear();
  size_t undo_count() const { return undo_.size(); }
  size_t redo_count() const { return redo_.size(); }

 private:
  std::vector<UndoBatch> undo_;
  std::vector<UndoBatch> redo_;
};

UndoBatch diff_state_maps(const std::string& label,
                          const std::map<std::string, std::string>& before,
                          const std::map<std::string, std::string>& after);
std::string describe_undo_batch(const UndoBatch& batch);

}  // namespace wayvault
