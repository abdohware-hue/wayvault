# WayVault

WayVault is a C++17 library and command-line utility for maintaining compact offline tile vaults. It reads a binary vault format used by small map preview tools to keep tile payloads, string tables, metadata, command streams, and layout notes together in one portable file.

The project is organized as normal application code rather than a single parser file. The core library handles binary reading, section indexing, tile decoding, metadata summaries, route planning helpers, geometry utilities, atlas packing, label placement, text search, style resolution, image operations, storage planning, journal replay, and report rendering. The command-line tools use the same library APIs as downstream applications.

## Vault format

A `.wvl` file starts with a fixed header, followed by a section table and typed sections. Common sections include:

- `STRS`: null-terminated string table entries.
- `TILE`: fixed-size tile records with payload offsets.
- `DATA`: encoded tile payloads.
- `META`: metadata key/value references.
- `CMDS`: preview command stream.
- `INDX`: optional lookup records.

The format is intentionally simple to embed in offline tools, but the library keeps separate modules for parsing, validation, caching, reporting, and planning so callers can use only the parts they need.

## Build

```bash
cmake -S . -B build -GNinja
cmake --build build
ctest --test-dir build --output-on-failure
```

## Tools

Create a sample vault and print a summary:

```bash
./build/wayvault-make-sample /tmp/sample.wvl
./build/wayvault-dump /tmp/sample.wvl
```

## Developer checks

The repository includes deterministic unit tests, sample builders, and libFuzzer entry points used during reliability testing. The ClusterFuzzLite files live under `.clusterfuzzlite/`; they are developer infrastructure and are not required by normal users of the library.
