#include "wayvault/undo_stack.h"

#include <set>
#include <sstream>

namespace wayvault {

namespace {

void apply_patch(std::map<std::string, std::string>& state, const StatePatch& patch, bool forward) {
  const std::string& value = forward ? patch.after : patch.before;
  if (value.empty()) {
    state.erase(patch.key);
  } else {
    state[patch.key] = value;
  }
}

UndoBatch reverse_batch(const UndoBatch& batch) {
  UndoBatch reversed;
  reversed.label = batch.label;
  for (const StatePatch& patch : batch.patches) {
    reversed.patches.push_back(StatePatch{patch.key, patch.after, patch.before});
  }
  return reversed;
}

}  // namespace

void UndoStack::apply(std::map<std::string, std::string>& state, UndoBatch batch) {
  for (const StatePatch& patch : batch.patches) {
    apply_patch(state, patch, true);
  }
  undo_.push_back(std::move(batch));
  redo_.clear();
}

bool UndoStack::undo(std::map<std::string, std::string>& state) {
  if (undo_.empty()) return false;
  UndoBatch batch = undo_.back();
  undo_.pop_back();
  for (auto it = batch.patches.rbegin(); it != batch.patches.rend(); ++it) {
    apply_patch(state, *it, false);
  }
  redo_.push_back(std::move(batch));
  return true;
}

bool UndoStack::redo(std::map<std::string, std::string>& state) {
  if (redo_.empty()) return false;
  UndoBatch batch = redo_.back();
  redo_.pop_back();
  for (const StatePatch& patch : batch.patches) {
    apply_patch(state, patch, true);
  }
  undo_.push_back(std::move(batch));
  return true;
}

void UndoStack::clear() {
  undo_.clear();
  redo_.clear();
}

UndoBatch diff_state_maps(const std::string& label,
                          const std::map<std::string, std::string>& before,
                          const std::map<std::string, std::string>& after) {
  UndoBatch batch;
  batch.label = label;
  std::set<std::string> keys;
  for (const auto& kv : before) keys.insert(kv.first);
  for (const auto& kv : after) keys.insert(kv.first);
  for (const std::string& key : keys) {
    auto a = before.find(key);
    auto b = after.find(key);
    std::string av = a == before.end() ? std::string() : a->second;
    std::string bv = b == after.end() ? std::string() : b->second;
    if (av != bv) {
      batch.patches.push_back(StatePatch{key, av, bv});
    }
  }
  return batch;
}

std::string describe_undo_batch(const UndoBatch& batch) {
  std::ostringstream out;
  out << batch.label << " patches=" << batch.patches.size();
  for (const StatePatch& patch : batch.patches) {
    out << "\n  " << patch.key << ": " << patch.before << " -> " << patch.after;
  }
  return out.str();
}

}  // namespace wayvault
