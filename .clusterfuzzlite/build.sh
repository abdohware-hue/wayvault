#!/bin/bash -eu

PROJECT="$SRC"
BUILD="$WORK/build"
mkdir -p "$BUILD" "$OUT"

COMMON_SRC=(
  "$PROJECT/src/block_index.cpp"
  "$PROJECT/src/byte_reader.cpp"
  "$PROJECT/src/checksum.cpp"
  "$PROJECT/src/command_planner.cpp"
  "$PROJECT/src/container.cpp"
  "$PROJECT/src/diagnostics.cpp"
  "$PROJECT/src/hexdump.cpp"
  "$PROJECT/src/index.cpp"
  "$PROJECT/src/layout.cpp"
  "$PROJECT/src/manifest.cpp"
  "$PROJECT/src/metadata.cpp"
  "$PROJECT/src/query.cpp"
  "$PROJECT/src/render.cpp"
  "$PROJECT/src/stats.cpp"
  "$PROJECT/src/stream.cpp"
  "$PROJECT/src/string_table.cpp"
  "$PROJECT/src/tags.cpp"
  "$PROJECT/src/tile_cache.cpp"
  "$PROJECT/src/tile_decoder.cpp"
  "$PROJECT/src/validator.cpp"
)

COMMON_FLAGS=(
  -std=c++17
  -I"$PROJECT/include"
)

COMMON_OBJECTS=()
for src in "${COMMON_SRC[@]}"; do
  obj="$BUILD/$(basename "$src" .cpp).o"
  "$CXX" $CXXFLAGS "${COMMON_FLAGS[@]}" -c "$src" -o "$obj"
  COMMON_OBJECTS+=("$obj")
done

for target in container tile metadata stream; do
  "$CXX" $CXXFLAGS "${COMMON_FLAGS[@]}"     "$PROJECT/fuzz/${target}_fuzzer.cc"     "${COMMON_OBJECTS[@]}"     $LIB_FUZZING_ENGINE     -o "$OUT/${target}_fuzzer"
done

if [ -f "$PROJECT/fuzz/dictionary.txt" ]; then
  cp "$PROJECT/fuzz/dictionary.txt" "$OUT/wayvault.dict"
fi

for corpus_dir in "$PROJECT"/fuzz/corpus/*; do
  if [ -d "$corpus_dir" ]; then
    name="$(basename "$corpus_dir")"
    (cd "$corpus_dir" && zip -q -r "$OUT/${name}_seed_corpus.zip" .)
  fi
done
