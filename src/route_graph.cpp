#include "wayvault/route_graph.h"

#include <algorithm>
#include <limits>
#include <queue>
#include <set>
#include <sstream>

#include "wayvault/checksum.h"

namespace wayvault {

namespace {

struct QueueItem {
  uint32_t node = 0;
  double priority = 0.0;
  bool operator<(const QueueItem& other) const { return priority > other.priority; }
};

std::vector<uint32_t> reconstruct(uint32_t goal, const std::map<uint32_t, uint32_t>& came_from) {
  std::vector<uint32_t> path;
  uint32_t cur = goal;
  path.push_back(cur);
  while (true) {
    auto it = came_from.find(cur);
    if (it == came_from.end()) break;
    cur = it->second;
    path.push_back(cur);
  }
  std::reverse(path.begin(), path.end());
  return path;
}

double heuristic(const RouteGraph& graph, uint32_t a, uint32_t b) {
  const RouteNode* na = graph.find_node(a);
  const RouteNode* nb = graph.find_node(b);
  if (na == nullptr || nb == nullptr) return 0.0;
  return distance(na->point, nb->point);
}

}  // namespace

void RouteGraph::add_node(RouteNode node) {
  nodes_[node.id] = std::move(node);
}

void RouteGraph::add_edge(RouteEdge edge) {
  adjacency_.emplace(edge.from, edge);
  ++edge_count_;
}

const RouteNode* RouteGraph::find_node(uint32_t id) const {
  auto it = nodes_.find(id);
  return it == nodes_.end() ? nullptr : &it->second;
}

std::vector<RouteEdge> RouteGraph::outgoing(uint32_t id) const {
  std::vector<RouteEdge> out;
  auto range = adjacency_.equal_range(id);
  for (auto it = range.first; it != range.second; ++it) out.push_back(it->second);
  return out;
}

std::vector<uint32_t> RouteGraph::node_ids() const {
  std::vector<uint32_t> out;
  for (const auto& kv : nodes_) out.push_back(kv.first);
  return out;
}

RoutePath shortest_path(const RouteGraph& graph, uint32_t start, uint32_t goal, bool use_geometry_heuristic) {
  RoutePath result;
  if (graph.find_node(start) == nullptr || graph.find_node(goal) == nullptr) return result;
  std::priority_queue<QueueItem> open;
  std::map<uint32_t, double> cost_so_far;
  std::map<uint32_t, uint32_t> came_from;
  open.push({start, 0.0});
  cost_so_far[start] = 0.0;
  while (!open.empty()) {
    QueueItem item = open.top();
    open.pop();
    if (item.node == goal) break;
    double base = cost_so_far[item.node];
    for (const RouteEdge& edge : graph.outgoing(item.node)) {
      if (graph.find_node(edge.to) == nullptr) continue;
      double penalty = (edge.flags & 1u) ? edge.cost * 0.25 : 0.0;
      double next_cost = base + edge.cost + penalty;
      auto it = cost_so_far.find(edge.to);
      if (it == cost_so_far.end() || next_cost < it->second) {
        cost_so_far[edge.to] = next_cost;
        came_from[edge.to] = item.node;
        double h = use_geometry_heuristic ? heuristic(graph, edge.to, goal) : 0.0;
        open.push({edge.to, next_cost + h});
      }
    }
  }
  auto goal_cost = cost_so_far.find(goal);
  if (goal_cost == cost_so_far.end()) return result;
  result.nodes = reconstruct(goal, came_from);
  result.total_cost = goal_cost->second;
  for (size_t i = 1; i < result.nodes.size(); ++i) {
    for (const RouteEdge& edge : graph.outgoing(result.nodes[i - 1])) {
      if (edge.to == result.nodes[i]) {
        result.total_length += edge.length;
        break;
      }
    }
  }
  result.found = true;
  return result;
}

std::vector<std::vector<uint32_t>> connected_components(const RouteGraph& graph) {
  std::set<uint32_t> unseen;
  for (uint32_t id : graph.node_ids()) unseen.insert(id);
  std::vector<std::vector<uint32_t>> components;
  while (!unseen.empty()) {
    uint32_t start = *unseen.begin();
    unseen.erase(start);
    std::vector<uint32_t> component;
    std::queue<uint32_t> q;
    q.push(start);
    while (!q.empty()) {
      uint32_t id = q.front();
      q.pop();
      component.push_back(id);
      for (const RouteEdge& edge : graph.outgoing(id)) {
        auto it = unseen.find(edge.to);
        if (it != unseen.end()) {
          unseen.erase(it);
          q.push(edge.to);
        }
      }
    }
    components.push_back(component);
  }
  return components;
}

RouteGraph build_grid_graph(uint32_t width, uint32_t height, double spacing) {
  RouteGraph graph;
  for (uint32_t y = 0; y < height; ++y) {
    for (uint32_t x = 0; x < width; ++x) {
      uint32_t id = y * width + x;
      graph.add_node(RouteNode{id, Point2{spacing * x, spacing * y}, "n" + std::to_string(id)});
    }
  }
  for (uint32_t y = 0; y < height; ++y) {
    for (uint32_t x = 0; x < width; ++x) {
      uint32_t id = y * width + x;
      if (x + 1 < width) {
        graph.add_edge(RouteEdge{id, id + 1, spacing, spacing, 0});
        graph.add_edge(RouteEdge{id + 1, id, spacing, spacing, 0});
      }
      if (y + 1 < height) {
        graph.add_edge(RouteEdge{id, id + width, spacing, spacing, 0});
        graph.add_edge(RouteEdge{id + width, id, spacing, spacing, 0});
      }
    }
  }
  return graph;
}

std::string describe_route(const RouteGraph& graph, const RoutePath& path) {
  std::ostringstream out;
  if (!path.found) return "route not found";
  out << "cost=" << path.total_cost << " length=" << path.total_length << " nodes=" << path.nodes.size();
  for (uint32_t id : path.nodes) {
    const RouteNode* node = graph.find_node(id);
    out << "\n  " << id;
    if (node != nullptr && !node->name.empty()) out << " " << node->name;
  }
  return out.str();
}

uint32_t route_hash(const RoutePath& path) {
  uint32_t state = 2166136261u;
  state = rotate_mix(state, path.found ? 1u : 0u);
  for (uint32_t id : path.nodes) state = rotate_mix(state, id);
  state = rotate_mix(state, static_cast<uint32_t>(path.total_cost * 1000.0));
  return state;
}

}  // namespace wayvault
