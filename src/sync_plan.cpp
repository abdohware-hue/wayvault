#include "wayvault/sync_plan.h"

#include <algorithm>
#include <sstream>

#include "wayvault/checksum.h"

namespace wayvault {

namespace {

std::vector<std::string> split(const std::string& line) {
  std::vector<std::string> out;
  std::string cur;
  for (char c : line) {
    if (c == '|') {
      out.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  out.push_back(cur);
  return out;
}

std::map<std::string, SyncEndpointState> map_state(const std::vector<SyncEndpointState>& state) {
  std::map<std::string, SyncEndpointState> out;
  for (const SyncEndpointState& item : state) out[item.key] = item;
  return out;
}

void add_operation(SyncPlan& plan, SyncOperation op) {
  if (op.action == "upload") plan.upload_bytes += op.transfer_bytes;
  if (op.action == "download") plan.download_bytes += op.transfer_bytes;
  if (op.action == "conflict") ++plan.conflicts;
  plan.operations.push_back(std::move(op));
}

}  // namespace

std::vector<SyncEndpointState> parse_sync_state(const std::string& text) {
  std::vector<SyncEndpointState> state;
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') continue;
    auto cells = split(line);
    if (cells.size() < 5) continue;
    SyncEndpointState item;
    item.key = cells[0];
    item.revision = std::stoull(cells[1]);
    item.checksum = static_cast<uint32_t>(std::stoul(cells[2]));
    item.size = std::stoull(cells[3]);
    item.tombstone = cells[4] == "1" || cells[4] == "true";
    state.push_back(std::move(item));
  }
  std::sort(state.begin(), state.end(), [](const SyncEndpointState& a, const SyncEndpointState& b) {
    return a.key < b.key;
  });
  return state;
}

std::string write_sync_state(const std::vector<SyncEndpointState>& state) {
  std::ostringstream out;
  for (const SyncEndpointState& item : state) {
    out << item.key << '|' << item.revision << '|' << item.checksum << '|'
        << item.size << '|' << (item.tombstone ? 1 : 0) << '\n';
  }
  return out.str();
}

SyncPlan build_sync_plan(const std::vector<SyncEndpointState>& local, const std::vector<SyncEndpointState>& remote, bool prefer_remote) {
  SyncPlan plan;
  auto left = map_state(local);
  auto right = map_state(remote);
  std::map<std::string, bool> keys;
  for (const auto& kv : left) keys[kv.first] = true;
  for (const auto& kv : right) keys[kv.first] = true;
  for (const auto& kv : keys) {
    const std::string& key = kv.first;
    auto lit = left.find(key);
    auto rit = right.find(key);
    if (lit == left.end() && rit != right.end()) {
      if (!rit->second.tombstone) add_operation(plan, {key, "download", 0, rit->second.revision, rit->second.size, 10});
      continue;
    }
    if (rit == right.end() && lit != left.end()) {
      if (!lit->second.tombstone) add_operation(plan, {key, "upload", lit->second.revision, 0, lit->second.size, 10});
      continue;
    }
    const SyncEndpointState& l = lit->second;
    const SyncEndpointState& r = rit->second;
    if (l.revision == r.revision && l.checksum == r.checksum && l.tombstone == r.tombstone) continue;
    if (l.tombstone != r.tombstone) {
      if (prefer_remote) add_operation(plan, {key, r.tombstone ? "delete-local" : "download", l.revision, r.revision, r.size, 50});
      else add_operation(plan, {key, l.tombstone ? "delete-remote" : "upload", l.revision, r.revision, l.size, 50});
      continue;
    }
    if (l.revision > r.revision && l.checksum != r.checksum) {
      add_operation(plan, {key, "upload", l.revision, r.revision, l.size, 20});
    } else if (r.revision > l.revision && l.checksum != r.checksum) {
      add_operation(plan, {key, "download", l.revision, r.revision, r.size, 20});
    } else if (l.checksum != r.checksum) {
      add_operation(plan, {key, "conflict", l.revision, r.revision, std::max(l.size, r.size), 100});
    }
  }
  std::sort(plan.operations.begin(), plan.operations.end(), [](const SyncOperation& a, const SyncOperation& b) {
    if (a.priority != b.priority) return a.priority > b.priority;
    return a.key < b.key;
  });
  return plan;
}

std::vector<SyncOperation> filter_sync_operations(const SyncPlan& plan, const std::string& action) {
  std::vector<SyncOperation> out;
  for (const SyncOperation& op : plan.operations) {
    if (action.empty() || op.action == action) out.push_back(op);
  }
  return out;
}

std::map<std::string, size_t> sync_action_counts(const SyncPlan& plan) {
  std::map<std::string, size_t> counts;
  for (const SyncOperation& op : plan.operations) counts[op.action]++;
  return counts;
}

std::string summarize_sync_plan(const SyncPlan& plan) {
  std::ostringstream out;
  out << "operations=" << plan.operations.size() << " upload_bytes=" << plan.upload_bytes
      << " download_bytes=" << plan.download_bytes << " conflicts=" << plan.conflicts;
  for (const auto& kv : sync_action_counts(plan)) out << ' ' << kv.first << '=' << kv.second;
  return out.str();
}

uint32_t sync_plan_hash(const SyncPlan& plan) {
  uint32_t state = rotate_mix(0x8d442cbfu, static_cast<uint32_t>(plan.operations.size()));
  state = rotate_mix(state, static_cast<uint32_t>(plan.upload_bytes));
  state = rotate_mix(state, static_cast<uint32_t>(plan.download_bytes));
  state = rotate_mix(state, static_cast<uint32_t>(plan.conflicts));
  for (const SyncOperation& op : plan.operations) {
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(op.key.data()), op.key.size()));
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(op.action.data()), op.action.size()));
    state = rotate_mix(state, static_cast<uint32_t>(op.local_revision));
    state = rotate_mix(state, static_cast<uint32_t>(op.remote_revision));
    state = rotate_mix(state, static_cast<uint32_t>(op.transfer_bytes));
    state = rotate_mix(state, op.priority);
  }
  return state;
}

}  // namespace wayvault
