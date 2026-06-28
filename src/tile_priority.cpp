#include "wayvault/tile_priority.h"

#include <algorithm>
#include <cmath>
#include <sstream>

#include "wayvault/checksum.h"

namespace wayvault {

namespace {

uint64_t tile_key(TileCoord coord) {
  return (static_cast<uint64_t>(coord.z) << 58) ^ (static_cast<uint64_t>(coord.x) << 29) ^ coord.y;
}

}  // namespace

std::vector<TileRequest> build_view_requests(const ViewportState& viewport, uint32_t bytes_per_tile) {
  std::vector<TileRequest> requests;
  auto tiles = visible_tiles(viewport, 4096);
  requests.reserve(tiles.size());
  for (const ViewTile& tile : tiles) {
    TileRequest request;
    request.coord = tile.coord;
    request.reason = tile.partly_visible ? "visible" : "prefetch";
    request.bytes_estimate = bytes_per_tile;
    request.distance_to_focus = tile.priority;
    request.pinned = tile.priority < 128.0;
    requests.push_back(std::move(request));
  }
  return requests;
}

double tile_request_score(const TileRequest& request, uint32_t target_zoom) {
  double score = request.distance_to_focus;
  score += std::abs(static_cast<int>(request.coord.z) - static_cast<int>(target_zoom)) * 4096.0;
  score += request.miss_count * 64.0;
  if (request.pinned) score -= 2048.0;
  if (request.reason == "visible") score -= 256.0;
  return score;
}

RequestBatch select_request_batch(std::vector<TileRequest> requests, RequestBudget budget, uint32_t target_zoom) {
  RequestBatch batch;
  std::sort(requests.begin(), requests.end(), [target_zoom](const TileRequest& a, const TileRequest& b) {
    double as = tile_request_score(a, target_zoom);
    double bs = tile_request_score(b, target_zoom);
    if (as != bs) return as < bs;
    return tile_key(a.coord) < tile_key(b.coord);
  });
  uint32_t bytes = 0;
  for (const TileRequest& request : requests) {
    if (batch.requests.size() >= budget.max_requests) {
      ++batch.dropped;
      continue;
    }
    if (request.bytes_estimate > budget.max_bytes || bytes > budget.max_bytes - request.bytes_estimate) {
      ++batch.dropped;
      continue;
    }
    if (budget.prefer_pinned && !request.pinned && batch.requests.size() + batch.dropped < requests.size() / 4) {
      ++batch.dropped;
      continue;
    }
    batch.requests.push_back(request);
    bytes += request.bytes_estimate;
  }
  batch.estimated_bytes = bytes;
  return batch;
}

std::map<uint32_t, size_t> count_requests_by_zoom(const std::vector<TileRequest>& requests) {
  std::map<uint32_t, size_t> counts;
  for (const TileRequest& request : requests) counts[request.coord.z]++;
  return counts;
}

std::string summarize_request_batch(const RequestBatch& batch) {
  std::ostringstream out;
  out << "requests=" << batch.requests.size() << " bytes=" << batch.estimated_bytes << " dropped=" << batch.dropped;
  auto counts = count_requests_by_zoom(batch.requests);
  for (const auto& kv : counts) out << " z" << kv.first << '=' << kv.second;
  return out.str();
}

uint32_t request_batch_hash(const RequestBatch& batch) {
  uint32_t state = rotate_mix(0x9146c2b5u, static_cast<uint32_t>(batch.requests.size()));
  state = rotate_mix(state, batch.estimated_bytes);
  state = rotate_mix(state, batch.dropped);
  for (const TileRequest& request : batch.requests) {
    state = rotate_mix(state, request.coord.z);
    state = rotate_mix(state, request.coord.x);
    state = rotate_mix(state, request.coord.y);
    state = rotate_mix(state, request.bytes_estimate);
    state = rotate_mix(state, request.pinned ? 1u : 0u);
  }
  return state;
}

}  // namespace wayvault
