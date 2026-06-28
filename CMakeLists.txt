cmake_minimum_required(VERSION 3.16)
project(wayvault LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(wayvault
  src/block_index.cpp
  src/byte_reader.cpp
  src/checksum.cpp
  src/command_planner.cpp
  src/container.cpp
  src/diagnostics.cpp
  src/hexdump.cpp
  src/index.cpp
  src/io.cpp
  src/layout.cpp
  src/manifest.cpp
  src/metadata.cpp
  src/query.cpp
  src/render.cpp
  src/sample_builder.cpp
  src/stats.cpp
  src/stream.cpp
  src/string_table.cpp
  src/tags.cpp
  src/tile_cache.cpp
  src/tile_decoder.cpp
  src/validator.cpp
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
