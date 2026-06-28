cmake_minimum_required(VERSION 3.16)
project(wayvault LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(wayvault
  src/annotation_layer.cpp
  src/archive_index.cpp
  src/asset_bundle.cpp
  src/atlas_packer.cpp
  src/block_index.cpp
  src/byte_reader.cpp
  src/cache_warmer.cpp
  src/change_feed.cpp
  src/checksum.cpp
  src/color_ramp.cpp
  src/command_planner.cpp
  src/compression.cpp
  src/container.cpp
  src/csv_table.cpp
  src/diagnostics.cpp
  src/diff_report.cpp
  src/event_log.cpp
  src/feature_flags.cpp
  src/field_codec.cpp
  src/geo_tile.cpp
  src/geometry.cpp
  src/heatmap.cpp
  src/hexdump.cpp
  src/index.cpp
  src/io.cpp
  src/journal.cpp
  src/json_writer.cpp
  src/label_layout.cpp
  src/layer_stack.cpp
  src/layout.cpp
  src/legend_builder.cpp
  src/manifest.cpp
  src/manifest_store.cpp
  src/metadata.cpp
  src/network_trace.cpp
  src/packet_window.cpp
  src/path_matcher.cpp
  src/permissions.cpp
  src/projection.cpp
  src/query.cpp
  src/query_language.cpp
  src/raster_ops.cpp
  src/rebuild_scheduler.cpp
  src/render.cpp
  src/report.cpp
  src/resource_quota.cpp
  src/retention_policy.cpp
  src/route_graph.cpp
  src/sample_builder.cpp
  src/scale_bar.cpp
  src/scene_graph.cpp
  src/schema_validator.cpp
  src/spatial_index.cpp
  src/stats.cpp
  src/storage_plan.cpp
  src/stream.cpp
  src/string_metrics.cpp
  src/string_table.cpp
  src/style_sheet.cpp
  src/symbolizer.cpp
  src/sync_plan.cpp
  src/tags.cpp
  src/text_index.cpp
  src/tile_cache.cpp
  src/tile_catalog.cpp
  src/tile_decoder.cpp
  src/tile_matrix.cpp
  src/tile_priority.cpp
  src/tile_recipe.cpp
  src/tile_sampler.cpp
  src/time_series.cpp
  src/undo_stack.cpp
  src/validator.cpp
  src/vault_analyzer.cpp
  src/viewport.cpp
  src/workflow_engine.cpp

)

target_include_directories(wayvault PUBLIC include)
target_compile_options(wayvault PRIVATE -Wall -Wextra -Wpedantic)

add_executable(wayvault-dump tools/wayvault_dump.cpp)
target_link_libraries(wayvault-dump PRIVATE wayvault)

add_executable(wayvault-make-sample tools/make_sample.cpp)
target_link_libraries(wayvault-make-sample PRIVATE wayvault)

enable_testing()
add_executable(wayvault-test-basic tests/test_basic.cpp)
target_link_libraries(wayvault-test-basic PRIVATE wayvault)
add_test(NAME wayvault-test-basic COMMAND wayvault-test-basic)

add_executable(wayvault-test-error-paths tests/test_error_paths.cpp)
target_link_libraries(wayvault-test-error-paths PRIVATE wayvault)
add_test(NAME wayvault-test-error-paths COMMAND wayvault-test-error-paths)

add_executable(wayvault-test-expanded tests/test_expanded.cpp)
target_link_libraries(wayvault-test-expanded PRIVATE wayvault)
add_test(NAME wayvault-test-expanded COMMAND wayvault-test-expanded)
