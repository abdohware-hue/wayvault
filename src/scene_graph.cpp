#include "wayvault/scene_graph.h"

#include <algorithm>
#include <map>
#include <set>
#include <functional>

#include "wayvault/checksum.h"

namespace wayvault {

SceneTransform multiply_transform(SceneTransform a, SceneTransform b) {
  SceneTransform out;
  out.a = a.a * b.a + a.c * b.b;
  out.b = a.b * b.a + a.d * b.b;
  out.c = a.a * b.c + a.c * b.d;
  out.d = a.b * b.c + a.d * b.d;
  out.tx = a.a * b.tx + a.c * b.ty + a.tx;
  out.ty = a.b * b.tx + a.d * b.ty + a.ty;
  return out;
}

Point2 apply_transform(SceneTransform t, Point2 p) {
  return {t.a * p.x + t.c * p.y + t.tx, t.b * p.x + t.d * p.y + t.ty};
}

Bounds2 transform_bounds(SceneTransform t, Bounds2 bounds) {
  if (!bounds.valid) return bounds;
  std::vector<Point2> corners = {
      {bounds.min_x, bounds.min_y}, {bounds.max_x, bounds.min_y},
      {bounds.max_x, bounds.max_y}, {bounds.min_x, bounds.max_y}};
  for (Point2& p : corners) p = apply_transform(t, p);
  return bounds_for_points(corners);
}

std::vector<uint32_t> scene_draw_order(const std::vector<SceneNode>& nodes) {
  std::map<uint32_t, std::vector<uint32_t>> children;
  std::set<uint32_t> all;
  std::set<uint32_t> has_parent;
  for (const SceneNode& node : nodes) {
    all.insert(node.id);
    if (node.parent != 0) {
      children[node.parent].push_back(node.id);
      has_parent.insert(node.id);
    }
  }
  for (auto& kv : children) std::sort(kv.second.begin(), kv.second.end());
  std::vector<uint32_t> order;
  std::function<void(uint32_t)> visit = [&](uint32_t id) {
    order.push_back(id);
    for (uint32_t child : children[id]) visit(child);
  };
  for (uint32_t id : all) {
    if (!has_parent.count(id)) visit(id);
  }
  return order;
}

std::vector<FlattenedNode> flatten_scene(const std::vector<SceneNode>& nodes) {
  std::map<uint32_t, SceneNode> by_id;
  for (const SceneNode& node : nodes) by_id[node.id] = node;
  std::map<uint32_t, std::vector<uint32_t>> children;
  for (const SceneNode& node : nodes) children[node.parent].push_back(node.id);
  std::vector<FlattenedNode> out;
  std::function<void(uint32_t, SceneTransform, std::string)> visit = [&](uint32_t id, SceneTransform parent, std::string path) {
    auto it = by_id.find(id);
    if (it == by_id.end()) return;
    const SceneNode& node = it->second;
    SceneTransform world = multiply_transform(parent, node.transform);
    std::string next_path = path.empty() ? node.name : path + "/" + node.name;
    out.push_back(FlattenedNode{id, next_path, transform_bounds(world, node.local_bounds), world});
    for (uint32_t child : children[id]) visit(child, world, next_path);
  };
  for (uint32_t root : children[0]) visit(root, SceneTransform{}, "");
  return out;
}

uint32_t scene_hash(const std::vector<FlattenedNode>& nodes) {
  uint32_t state = 2166136261u;
  for (const FlattenedNode& node : nodes) {
    state = rotate_mix(state, node.id);
    state = rotate_mix(state, fnv1a32(reinterpret_cast<const uint8_t*>(node.path.data()), node.path.size()));
    if (node.world_bounds.valid) {
      state = rotate_mix(state, static_cast<uint32_t>(node.world_bounds.min_x * 1024.0));
      state = rotate_mix(state, static_cast<uint32_t>(node.world_bounds.max_y * 1024.0));
    }
  }
  return state;
}

}  // namespace wayvault
