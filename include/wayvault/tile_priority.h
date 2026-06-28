#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "wayvault/geo_tile.h"
#include "wayvault/viewport.h"

namespace wayvault {

struct TileRequest {
  TileCoord coord;
  std::string reason;
  uint32_t bytes_estimate = 0;
  uint32_t miss_count = 0;
  double distance_to_focus = 0.0;
  bool pinned = false;
};

struct RequestBatch {
  std::vector<TileRequest> requests;
  uint32_t estimated_bytes = 0;
  uint32_t dropped = 0;
};

struct RequestBudget {
  uint32_t max_requests = 64;
  uint32_t max_bytes = 4 * 1024 * 1024;
  bool prefer_pinned = true;
};

std::vector<TileRequest> build_view_requests(const ViewportState& viewport, uint32_t bytes_per_tile = 32768);
double tile_request_score(const TileRequest& request, uint32_t target_zoom);
RequestBatch select_request_batch(std::vector<TileRequest> requests, RequestBudget budget, uint32_t target_zoom);
std::map<uint32_t, size_t> count_requests_by_zoom(const std::vector<TileRequest>& requests);
std::string summarize_request_batch(const RequestBatch& batch);
uint32_t request_batch_hash(const RequestBatch& batch);

}  // namespace wayvault
