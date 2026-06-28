# WayVault

WayVault is a small C++17 library and command-line utility for reading a compact
binary tile vault format used by offline map and scene tooling. A vault contains
section tables, a string table, tile descriptors, packed image blocks, metadata,
and optional command streams used by tooling to preview several tiles through a
shared decode cache.

This repository is intentionally structured like a real parser/decoder project:

- `include/wayvault/` contains public library headers.
- `src/` contains parser, index, metadata, tile decoding, cache, stream, and CLI logic.
- `tools/wayvault_dump.cpp` provides a normal command-line entry point.
- `tests/` contains basic deterministic behavior tests.
- `fuzz/` contains libFuzzer harnesses that call real library APIs.
- `.clusterfuzzlite/` contains the ClusterFuzzLite build integration.

## Format overview

A `.wvl` file starts with a 16-byte header:

```text
magic:         4 bytes, "WVLT"
version:       uint16 little-endian
flags:         uint16 little-endian
section_count: uint16 little-endian
reserved:      uint16 little-endian
table_offset:  uint32 little-endian
```

The section table contains 20-byte entries:

```text
tag:        uint32 little-endian, four ASCII bytes
offset:     uint32 little-endian, absolute file offset
size:       uint32 little-endian
count_hint: uint32 little-endian
flags:      uint32 little-endian
```

Common section tags are:

- `STRS`: string table.
- `TILE`: fixed-size tile records.
- `DATA`: tile payload bytes.
- `META`: metadata key/value references.
- `CMDS`: preview command stream.
- `INDX`: optional index records.

The parser validates normal container boundaries and throws `wayvault::ParseError`
for ordinary malformed input. Fuzz harnesses catch expected parser errors and do
not treat them as crashes.

## Build locally

```bash
cmake -S . -B build -GNinja
cmake --build build
ctest --test-dir build --output-on-failure
```

## Run the sample tool

```bash
./build/wayvault-make-sample /tmp/sample.wvl
./build/wayvault-dump /tmp/sample.wvl
```

## Fuzzing integration

ClusterFuzzLite should run `.clusterfuzzlite/build.sh`. The script builds all
fuzz targets into `$OUT` and links them with `$LIB_FUZZING_ENGINE`.

Current harnesses:

- `container_fuzzer`: parses full containers and builds the lookup index.
- `tile_fuzzer`: decodes tile payloads through normal decoder APIs.
- `metadata_fuzzer`: parses metadata and string-table references.
- `stream_fuzzer`: executes command-stream sessions through the shared tile cache.

Seed corpora are under `fuzz/corpus/<target>/`.
