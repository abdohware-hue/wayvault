# WayVault binary notes

WayVault files are deliberately small and friendly to fuzzing. Most fields are
little-endian. Unknown sections are retained in the parsed container so tooling
can round-trip or inspect files even when the library does not know every tag.

## Header

The parser accepts versions 1 through 3. Later versions are rejected. The section
table offset may be larger than the fixed header to support future embedded
prelude data, but it must still fit inside the file and leave enough room for all
section entries.

## String table (`STRS`)

```text
count: uint16
repeat count:
  length: uint16
  bytes:  length bytes, UTF-8 by convention
```

The library stores strings as `std::string`. Metadata and tile records reference
strings by index. Missing or out-of-range string references are treated as empty
names by the display layer, not as fatal parser errors.

## Tile table (`TILE`)

```text
count: uint16
repeat count:
  id:          uint32
  name_index:  uint16
  width:       uint16
  height:      uint16
  codec:       uint8
  layer:       uint8
  flags:       uint16
  data_offset: uint32, relative to the `DATA` section
  data_size:   uint32
  aux:         uint32
```

Supported codecs are:

- `0`: raw bytes, one byte per pixel.
- `1`: tiny RLE byte pairs `(run, value)`.
- `2`: delta rows, where each byte accumulates with the previous byte.
- `3`: palette-indexed blocks with a local palette header.

Invalid tile dimensions, payload ranges, and truncated encodings are ordinary
parse/decode errors.

## Command stream (`CMDS`)

Command streams are used by preview tools to reuse a tile decode cache while
walking a set of tiles.

```text
count: uint16
repeat count:
  op:      uint8
  tile_id: uint32
  arg:     uint16
  repeat:  uint8
```

Known opcodes:

- `1`: decode one tile and fold bytes into the session checksum.
- `2`: retain a view to the tile most recently decoded for later sampling.
- `3`: sample the retained view.
- `4`: walk several tile identifiers through the cache.
- `5`: clear transient stream counters.

The stream harness intentionally reaches cache lifecycle behavior instead of
only running a shallow header parser.
