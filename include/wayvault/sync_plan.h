#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace wayvault {

struct SyncEndpointState {
  std::string key;
  uint64_t revision = 0;
  uint32_t checksum = 0;
  uint64_t size = 0;
  bool tombstone = false;
};

struct SyncOperation {
  std::string key;
  std::string action;
  uint64_t local_revision = 0;
  uint64_t remote_revision = 0;
  uint64_t transfer_bytes = 0;
  uint32_t priority = 0;
};

struct SyncPlan {
  std::vector<SyncOperation> operations;
  uint64_t upload_bytes = 0;
  uint64_t download_bytes = 0;
  size_t conflicts = 0;
};

std::vector<SyncEndpointState> parse_sync_state(const std::string& text);
std::string write_sync_state(const std::vector<SyncEndpointState>& state);
SyncPlan build_sync_plan(const std::vector<SyncEndpointState>& local, const std::vector<SyncEndpointState>& remote, bool prefer_remote);
std::vector<SyncOperation> filter_sync_operations(const SyncPlan& plan, const std::string& action);
std::map<std::string, size_t> sync_action_counts(const SyncPlan& plan);
std::string summarize_sync_plan(const SyncPlan& plan);
uint32_t sync_plan_hash(const SyncPlan& plan);

}  // namespace wayvault
