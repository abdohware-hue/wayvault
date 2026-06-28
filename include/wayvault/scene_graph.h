#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "wayvault/geometry.h"

namespace wayvault {

struct SceneTransform {
  double a = 1.0;
  double b = 0.0;
  double c = 0.0;
  double d = 1.0;
  double tx = 0.0;
  double ty = 0.0;
};

struct SceneNode {
  uint32_t id = 0;
  uint32_t parent = 0;
  std::string name;
  SceneTransform transform;
  Bounds2 local_bounds;
};

struct FlattenedNode {
  uint32_t id = 0;
  std::string path;
  Bounds2 world_bounds;
  SceneTransform world_transform;
};

SceneTransform multiply_transform(SceneTransform a, SceneTransform b);
Point2 apply_transform(SceneTransform t, Point2 p);
Bounds2 transform_bounds(SceneTransform t, Bounds2 bounds);
std::vector<FlattenedNode> flatten_scene(const std::vector<SceneNode>& nodes);
std::vector<uint32_t> scene_draw_order(const std::vector<SceneNode>& nodes);
uint32_t scene_hash(const std::vector<FlattenedNode>& nodes);

}  // namespace wayvault
