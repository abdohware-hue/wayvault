#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "wayvault/geo_tile.h"
#include "wayvault/tile_priority.h"

namespace wayvault {

struct WarmupSeed {
  TileCoord coord;
  uint32_t frequency = 0;
  uint64_t last_used_ms = 0;
  std::string source;
};

struct WarmupPlan {
  std::vector<TileRequest> requests;
  uint32_t estimated_bytes = 0;
  std::vector<std::string> notes;
};

std::vector<WarmupSeed> parse_warmup_seeds(const std::string& text);
WarmupPlan build_warmup_plan(const std::vector<WarmupSeed>& seeds, RequestBudget budget, uint64_t now_ms);
std::vector<TileCoord> expand_warmup_neighbors(const std::vector<WarmupSeed>& seeds, uint32_t radius, size_t limit);
std::string write_warmup_plan(const WarmupPlan& plan);
uint32_t warmup_plan_hash(const WarmupPlan& plan);

}  // namespace wayvault
