#include <cassert>
#include <cmath>
#include <map>
#include <string>
#include <vector>

#include "wayvault/atlas_packer.h"
#include "wayvault/viewport.h"
#include "wayvault/tile_priority.h"
#include "wayvault/symbolizer.h"
#include "wayvault/spatial_index.h"
#include "wayvault/retention_policy.h"
#include "wayvault/projection.h"
#include "wayvault/network_trace.h"
#include "wayvault/field_codec.h"
#include "wayvault/asset_bundle.h"
#include "wayvault/annotation_layer.h"
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


  std::vector<GeoPoint> geo_path = {{31.2, 30.0}, {31.25, 30.05}, {31.3, 30.08}};
  auto projection_samples = sample_projection_path(geo_path, 10);
  assert(projection_samples.size() == geo_path.size());
  assert(approximate_path_meters(geo_path) > 1000.0);
  assert(projection_hash(projection_samples) != 0);

  ViewportState view{{31.25, 30.04}, 10, 800, 600, 15.0, 20.0};
  ScreenPoint screen = project_to_screen(view, geo_path.front());
  GeoPoint back = unproject_from_screen(view, screen);
  assert(std::abs(back.lon - geo_path.front().lon) < 0.1);
  auto view_tiles = visible_tiles(view, 128);
  assert(!view_tiles.empty());
  assert(viewport_hash(view, view_tiles) != 0);

  auto requests = build_view_requests(view, 4096);
  RequestBatch batch = select_request_batch(requests, RequestBudget{16, 65536, false}, view.zoom);
  assert(!batch.requests.empty());
  assert(request_batch_hash(batch) != 0);

  std::vector<uint8_t> field_bytes;
  write_string_field(1, "cairo", field_bytes);
  write_sint_field(2, -42, field_bytes);
  DecodedFields fields = decode_fields(field_bytes);
  assert(fields.complete);
  assert(filter_fields(fields, 1).size() == 1);
  assert(fields_hash(fields) != 0);

  SpatialIndex spatial = build_spatial_index(parse_spatial_features("a|0|0|10|10|1\nb|5|5|20|20|2\n"), 8.0);
  auto spatial_hits = spatial.search(SpatialQuery{Bounds2{4, 4, 12, 12, true}, 0xffffffffu, 10});
  assert(spatial_hits.size() == 2);
  assert(spatial_index_hash(spatial) != 0);

  BundleArchive bundle = build_bundle({{"style/main.json", "application/json", {1, 2, 3}, 0}, {"glyph/a.pbf", "application/x-protobuf", {4, 5}, 0}}, 4);
  BundleAsset extracted;
  assert(extract_asset(bundle, "style/main.json", extracted));
  BundleArchive reread = read_bundle_manifest(write_bundle_manifest(bundle), bundle.payload);
  assert(bundle_hash(reread) == bundle_hash(bundle));

  AnnotationLayer annotations = parse_annotation_layer("layer:notes\na|notes|1|2|kind=pin|0|10\nb|notes|5|6|kind=label|2|12\n");
  auto visible_notes = filter_annotations(annotations, Bounds2{0, 0, 10, 10, true}, 5);
  assert(visible_notes.size() == 2);
  assert(annotation_layer_hash(annotations) != 0);

  auto rules = parse_retention_rules("prefix=tile/ min_zoom=0 max_zoom=12 max_age=3600\nprefix=style/ pinned=1\n");
  std::vector<RetentionCandidate> candidates = {{"tile/a", 8, 1000, 7200, false}, {"style/main", 0, 300, 999999, false}};
  auto retention = plan_retention(candidates, rules, 500);
  assert(releasable_bytes(retention) >= 1000);
  assert(retention_hash(retention) != 0);

  Symbolizer symbolizer = parse_symbolizer("key=kind value=road symbol=line-road priority=9\nkey=kind value=water symbol=fill-water priority=5\n");
  auto symbols = symbolize_many(symbolizer, {{"f1", {{"kind", "road"}}}, {"f2", {{"kind", "park"}}}});
  assert(symbols.front().symbol == "line-road");
  assert(symbolizer_hash(symbols) != 0);

  auto trace = parse_trace_log("100,get,tile/a,200,200\n250,get,tile/b,300,404\n1000,put,tile/c,500,200\n");
  TraceSummary trace_summary = summarize_trace(trace);
  assert(trace_summary.transferred_bytes == 1000);
  assert(!slow_trace_events(trace, 500).empty());
  assert(trace_hash(trace) != 0);

  return 0;
}
