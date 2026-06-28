#include "wayvault/cache_warmer.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <sstream>

#include "wayvault/checksum.h"

namespace wayvault {

namespace {

std::vector<std::string> split(const std::string& line) {
  std::vector<std::string> out;
  std::string cur;
  for (char c : line) {
    if (c == ',') {
      out.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  out.push_back(cur);
  return out;
}

uint64_t key(TileCoord coord) {
  return (static_cast<uint64_t>(coord.z) << 58) ^ (static_cast<uint64_t>(coord.x) << 29) ^ coord.y;
}

}  // namespace

std::vector<WarmupSeed> parse_warmup_seeds(const std::string& text) {
  std::vector<WarmupSeed> seeds;
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') continue;
    auto cells = split(line);
    if (cells.size() < 6) continue;
    WarmupSeed seed;
    seed.coord.z = static_cast<uint32_t>(std::stoul(cells[0]));
    seed.coord.x = static_cast<uint32_t>(std::stoul(cells[1]));
    seed.coord.y = static_cast<uint32_t>(std::stoul(cells[2]));
    seed.frequency = static_cast<uint32_t>(std::stoul(cells[3]));
    seed.last_used_ms = std::stoull(cells[4]);
    seed.source = cells[5];
    seeds.push_back(std::move(seed));
  }
  return seeds;
}

std::vector<TileCoord> expand_warmup_neighbors(const std::vector<WarmupSeed>& seeds, uint32_t radius, size_t limit) {
  std::vector<TileCoord> coords;
  std::set<uint64_t> seen;
  for (const WarmupSeed& seed : seeds) {
    int r = static_cast<int>(std::min<uint32_t>(radius, 8));
    for (int dy = -r; dy <= r; ++dy) {
      for (int dx = -r; dx <= r; ++dx) {
        if (std::abs(dx) + std::abs(dy) > r) continue;
        int nx = static_cast<int>(seed.coord.x) + dx;
        int ny = static_cast<int>(seed.coord.y) + dy;
        if (nx < 0 || ny < 0) continue;
        TileCoord coord{seed.coord.z, static_cast<uint32_t>(nx), static_cast<uint32_t>(ny)};
        if (seen.insert(key(coord)).second) coords.push_back(coord);
        if (coords.size() >= limit) return coords;
      }
    }
  }
  return coords;
}

WarmupPlan build_warmup_plan(const std::vector<WarmupSeed>& seeds, RequestBudget budget, uint64_t now_ms) {
  std::vector<WarmupSeed> sorted = seeds;
  std::sort(sorted.begin(), sorted.end(), [now_ms](const WarmupSeed& a, const WarmupSeed& b) {
    uint64_t age_a = now_ms > a.last_used_ms ? now_ms - a.last_used_ms : 0;
    uint64_t age_b = now_ms > b.last_used_ms ? now_ms - b.last_used_ms : 0;
    uint64_t score_a = static_cast<uint64_t>(a.frequency) * 1000000ull / (age_a + 1);
    uint64_t score_b = static_cast<uint64_t>(b.frequency) * 1000000ull / (age_b + 1);
    if (score_a != score_b) return score_a > score_b;
    return key(a.coord) < key(b.coord);
  });
  std::vector<TileRequest> requests;
  for (const WarmupSeed& seed : sorted) {
    TileRequest request;
    request.coord = seed.coord;
    request.reason = seed.source.empty() ? "warmup" : seed.source;
    request.bytes_estimate = 32768;
    request.miss_count = seed.frequency > 100 ? 0 : 100 - seed.frequency;
    request.distance_to_focus = 0.0;
    request.pinned = seed.frequency > 10;
    requests.push_back(std::move(request));
  }
  RequestBatch selected = select_request_batch(requests, budget, sorted.empty() ? 0 : sorted.front().coord.z);
  WarmupPlan plan;
  plan.requests = std::move(selected.requests);
  plan.estimated_bytes = selected.estimated_bytes;
  if (selected.dropped) plan.notes.push_back("dropped=" + std::to_string(selected.dropped));
  if (plan.requests.empty()) plan.notes.push_back("no eligible warmup requests");
  return plan;
}

std::string write_warmup_plan(const WarmupPlan& plan) {
  std::ostringstream out;
  out << "estimated_bytes=" << plan.estimated_bytes << '\n';
  for (const std::string& note : plan.notes) out << "note=" << note << '\n';
  for (const TileRequest& request : plan.requests) {
    out << request.coord.z << '/' << request.coord.x << '/' << request.coord.y << ' '
        << request.reason << ' ' << request.bytes_estimate << '\n';
  }
  return out.str();
}

uint32_t warmup_plan_hash(const WarmupPlan& plan) {
  uint32_t state = rotate_mix(0x4a990621u, static_cast<uint32_t>(plan.requests.size()));
  state = rotate_mix(state, plan.estimated_bytes);
  for (const TileRequest& request : plan.requests) {
    state = rotate_mix(state, request.coord.z);
    state = rotate_mix(state, request.coord.x);
    state = rotate_mix(state, request.coord.y);
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(request.reason.data()), request.reason.size()));
  }
  for (const std::string& note : plan.notes) state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(note.data()), note.size()));
  return state;
}

}  // namespace wayvault
