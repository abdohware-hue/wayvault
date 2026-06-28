#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "wayvault/geometry.h"

namespace wayvault {

struct RouteNode {
  uint32_t id = 0;
  Point2 point;
  std::string name;
};

struct RouteEdge {
  uint32_t from = 0;
  uint32_t to = 0;
  double cost = 1.0;
  double length = 0.0;
  uint8_t flags = 0;
};

struct RoutePath {
  std::vector<uint32_t> nodes;
  double total_cost = 0.0;
  double total_length = 0.0;
  bool found = false;
};

class RouteGraph {
 public:
  void add_node(RouteNode node);
  void add_edge(RouteEdge edge);
  const RouteNode* find_node(uint32_t id) const;
  std::vector<RouteEdge> outgoing(uint32_t id) const;
  size_t node_count() const { return nodes_.size(); }
  size_t edge_count() const { return edge_count_; }
  std::vector<uint32_t> node_ids() const;

 private:
  std::map<uint32_t, RouteNode> nodes_;
  std::multimap<uint32_t, RouteEdge> adjacency_;
  size_t edge_count_ = 0;
};

RoutePath shortest_path(const RouteGraph& graph, uint32_t start, uint32_t goal, bool use_geometry_heuristic);
std::vector<std::vector<uint32_t>> connected_components(const RouteGraph& graph);
RouteGraph build_grid_graph(uint32_t width, uint32_t height, double spacing);
std::string describe_route(const RouteGraph& graph, const RoutePath& path);
uint32_t route_hash(const RoutePath& path);

}  // namespace wayvault
