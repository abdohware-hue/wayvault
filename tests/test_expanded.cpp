#include <cassert>
#include <cmath>
#include <map>
#include <string>
#include <vector>

#include "wayvault/atlas_packer.h"
#include "wayvault/compression.h"
#include "wayvault/geo_tile.h"
#include "wayvault/geometry.h"
#include "wayvault/journal.h"
#include "wayvault/label_layout.h"
#include "wayvault/query_language.h"
#include "wayvault/raster_ops.h"
#include "wayvault/route_graph.h"
#include "wayvault/scene_graph.h"
#include "wayvault/storage_plan.h"
#include "wayvault/style_sheet.h"
#include "wayvault/text_index.h"

using namespace wayvault;

int main() {
  std::vector<Point2> line = {{0, 0}, {3, 4}, {6, 4}, {9, 0}};
  PolylineStats stats = analyze_polyline(line);
  assert(stats.point_count == 4);
  assert(stats.length > 10.0);
  auto dense = densify_polyline(line, 2.0);
  assert(dense.size() > line.size());

  std::vector<Point2> square = {{0, 0}, {10, 0}, {10, 10}, {0, 10}};
  PolygonSummary poly = summarize_polygon(square);
  assert(poly.signed_area > 99.0 && poly.signed_area < 101.0);
  assert(point_in_polygon(square, {5, 5}));

  TileCoord coord = lon_lat_to_tile(31.2, 30.0, 8);
  std::string qk = to_quadkey(coord);
  TileCoord round = from_quadkey(qk);
  assert(same_tile(coord, round));
  assert(!neighbors(coord, true).empty());

  StyleSheet sheet = parse_style_sheet("layer=road; key=kind; value=primary; fill=#ffcc00; stroke=#804000; width=3; priority=5\n");
  std::map<std::string, std::string> tags = {{"kind", "primary"}};
  ResolvedStyle style = sheet.resolve(tags, "road");
  assert(style.width == 3.0);
  assert(style_hash(style) != 0);

  Bounds2 viewport{0, 0, 100, 100, true};
  std::vector<LabelAnchor> anchors = {{{20, 20}, "A", 1, 10, 0.0}, {{22, 20}, "B", 1, 9, 0.0}, {{70, 70}, "C", 1, 8, 0.0}};
  auto placements = place_labels(anchors, viewport, 10.0);
  assert(placements.size() == anchors.size());
  assert(placement_hash(placements) != 0);

  RouteGraph graph = build_grid_graph(3, 3, 1.0);
  RoutePath path = shortest_path(graph, 0, 8, true);
  assert(path.found);
  assert(path.nodes.front() == 0 && path.nodes.back() == 8);
  assert(route_hash(path) != 0);

  std::vector<uint8_t> bytes = {1, 1, 1, 2, 2, 9, 9, 9, 9};
  auto spans = rle_encode(bytes, 255);
  auto decoded = rle_decode_spans(spans, bytes.size());
  assert(decoded == bytes);
  auto packed = pack_nibbles({1, 2, 3, 4, 5});
  auto unpacked = unpack_nibbles(packed, 5);
  assert((unpacked == std::vector<uint8_t>{1, 2, 3, 4, 5}));

  TextIndex index;
  index.add_document("tile-a", "Cairo primary road label");
  index.add_document("tile-b", "river water label");
  auto hits = index.search("primary road", 3);
  assert(!hits.empty());
  assert(hits.front().document_id == "tile-a");

  std::vector<AtlasItem> items = {{"a", 16, 16, 1}, {"b", 20, 8, 1}, {"c", 7, 7, 1}};
  auto atlas = pack_atlas_pages(items, 64, 64);
  assert(atlas.size() == 3);
  assert(atlas_hash(atlas) != 0);

  GrayImage image = make_image(4, 4, 10);
  image.pixels[5] = 250;
  auto blurred = box_blur(image, 1);
  auto image_stats = analyze_image(blurred);
  assert(image_stats.max_value >= image_stats.min_value);
  assert(image_hash(blurred) != 0);

  QueryProgram program = parse_query_program("layer=2 && width>=8");
  assert(evaluate_query(program, {{"layer", "2"}, {"width", "16"}}));
  assert(!evaluate_query(program, {{"layer", "3"}, {"width", "16"}}));

  std::string journal_text = "1|set|name|north\n2|append|name|-east\n3|touch|status|ready\n";
  auto entries = parse_journal_text(journal_text);
  JournalState state = replay_journal(entries, true);
  assert(state.values["name"] == "north-east");

  std::vector<StorageExtent> extents = {{"a", 0, 10, 8}, {"b", 0, 4, 16}};
  StoragePlan plan = compact_storage_plan(extents, 8);
  assert(plan.total_size >= 14);
  assert(storage_plan_hash(plan) != 0);

  std::vector<SceneNode> scene = {
      {1, 0, "root", SceneTransform{}, Bounds2{0, 0, 10, 10, true}},
      {2, 1, "child", SceneTransform{1, 0, 0, 1, 5, 5}, Bounds2{0, 0, 2, 2, true}},
  };
  auto flat = flatten_scene(scene);
  assert(flat.size() == 2);
  assert(scene_hash(flat) != 0);
  return 0;
}
