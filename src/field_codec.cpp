#include "wayvault/field_codec.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "wayvault/checksum.h"

namespace wayvault {

namespace {

std::string hex_bytes(const std::vector<uint8_t>& bytes, size_t limit) {
  std::ostringstream out;
  out << std::hex << std::setfill('0');
  for (size_t i = 0; i < bytes.size() && i < limit; ++i) {
    if (i) out << ' ';
    out << std::setw(2) << static_cast<unsigned>(bytes[i]);
  }
  if (bytes.size() > limit) out << " ...";
  return out.str();
}

bool read_raw(FieldCursor& cursor, size_t count, std::vector<uint8_t>& out) {
  if (count > cursor.size || cursor.offset > cursor.size - count) return false;
  out.assign(cursor.data + cursor.offset, cursor.data + cursor.offset + count);
  cursor.offset += count;
  return true;
}

}  // namespace

uint64_t zigzag_encode(int64_t value) {
  return (static_cast<uint64_t>(value) << 1) ^ static_cast<uint64_t>(value >> 63);
}

int64_t zigzag_decode(uint64_t value) {
  return static_cast<int64_t>((value >> 1) ^ (~(value & 1) + 1));
}

bool write_varint(uint64_t value, std::vector<uint8_t>& out) {
  for (int i = 0; i < 10; ++i) {
    uint8_t byte = static_cast<uint8_t>(value & 0x7fu);
    value >>= 7;
    if (value) {
      out.push_back(static_cast<uint8_t>(byte | 0x80u));
    } else {
      out.push_back(byte);
      return true;
    }
  }
  return false;
}

bool read_varint(FieldCursor& cursor, uint64_t& value) {
  value = 0;
  uint32_t shift = 0;
  for (int i = 0; i < 10; ++i) {
    if (cursor.offset >= cursor.size) return false;
    uint8_t byte = cursor.data[cursor.offset++];
    value |= (static_cast<uint64_t>(byte & 0x7fu) << shift);
    if ((byte & 0x80u) == 0) return true;
    shift += 7;
  }
  return false;
}

void write_field_key(uint32_t id, uint32_t wire_type, std::vector<uint8_t>& out) {
  write_varint((static_cast<uint64_t>(id) << 3) | (wire_type & 7u), out);
}

bool read_field_key(FieldCursor& cursor, uint32_t& id, uint32_t& wire_type) {
  uint64_t key = 0;
  if (!read_varint(cursor, key)) return false;
  id = static_cast<uint32_t>(key >> 3);
  wire_type = static_cast<uint32_t>(key & 7u);
  return id != 0;
}

void write_bytes_field(uint32_t id, const std::vector<uint8_t>& bytes, std::vector<uint8_t>& out) {
  write_field_key(id, 2, out);
  write_varint(bytes.size(), out);
  out.insert(out.end(), bytes.begin(), bytes.end());
}

void write_string_field(uint32_t id, const std::string& value, std::vector<uint8_t>& out) {
  write_field_key(id, 2, out);
  write_varint(value.size(), out);
  out.insert(out.end(), value.begin(), value.end());
}

void write_sint_field(uint32_t id, int64_t value, std::vector<uint8_t>& out) {
  write_field_key(id, 0, out);
  write_varint(zigzag_encode(value), out);
}

DecodedFields decode_fields(const uint8_t* data, size_t size, size_t max_fields) {
  DecodedFields decoded;
  FieldCursor cursor{data, size, 0};
  while (cursor.offset < cursor.size) {
    if (decoded.entries.size() >= max_fields) {
      decoded.complete = false;
      decoded.warnings.push_back("field limit reached");
      break;
    }
    uint32_t id = 0;
    uint32_t wire = 0;
    size_t field_start = cursor.offset;
    if (!read_field_key(cursor, id, wire)) {
      decoded.complete = false;
      decoded.warnings.push_back("invalid field key at " + std::to_string(field_start));
      break;
    }
    FieldEntry entry;
    entry.id = id;
    entry.wire_type = wire;
    if (wire == 0) {
      uint64_t value = 0;
      if (!read_varint(cursor, value)) {
        decoded.complete = false;
        decoded.warnings.push_back("truncated varint field " + std::to_string(id));
        break;
      }
      write_varint(value, entry.bytes);
    } else if (wire == 1) {
      if (!read_raw(cursor, 8, entry.bytes)) {
        decoded.complete = false;
        decoded.warnings.push_back("truncated fixed64 field " + std::to_string(id));
        break;
      }
    } else if (wire == 2) {
      uint64_t length = 0;
      if (!read_varint(cursor, length) || length > cursor.size || cursor.offset > cursor.size - length) {
        decoded.complete = false;
        decoded.warnings.push_back("truncated bytes field " + std::to_string(id));
        break;
      }
      read_raw(cursor, static_cast<size_t>(length), entry.bytes);
    } else if (wire == 5) {
      if (!read_raw(cursor, 4, entry.bytes)) {
        decoded.complete = false;
        decoded.warnings.push_back("truncated fixed32 field " + std::to_string(id));
        break;
      }
    } else {
      decoded.complete = false;
      decoded.warnings.push_back("unknown wire type " + std::to_string(wire));
      break;
    }
    decoded.entries.push_back(std::move(entry));
  }
  return decoded;
}

DecodedFields decode_fields(const std::vector<uint8_t>& data, size_t max_fields) {
  return decode_fields(data.data(), data.size(), max_fields);
}

std::string field_to_string(const FieldEntry& entry) {
  std::ostringstream out;
  out << "field " << entry.id << " wire " << entry.wire_type << " size " << entry.bytes.size();
  if (entry.wire_type == 0) {
    FieldCursor cursor{entry.bytes.data(), entry.bytes.size(), 0};
    uint64_t value = 0;
    if (read_varint(cursor, value)) out << " varint " << zigzag_decode(value);
  } else if (entry.wire_type == 2) {
    bool printable = std::all_of(entry.bytes.begin(), entry.bytes.end(), [](uint8_t c) {
      return c >= 32 && c < 127;
    });
    if (printable) out << " text \"" << std::string(entry.bytes.begin(), entry.bytes.end()) << '"';
    else out << " hex " << hex_bytes(entry.bytes, 12);
  }
  return out.str();
}

std::vector<FieldEntry> filter_fields(const DecodedFields& fields, uint32_t id) {
  std::vector<FieldEntry> out;
  for (const FieldEntry& entry : fields.entries) {
    if (entry.id == id) out.push_back(entry);
  }
  return out;
}

uint32_t fields_hash(const DecodedFields& fields) {
  uint32_t state = rotate_mix(0x461d05d7u, static_cast<uint32_t>(fields.entries.size()));
  for (const FieldEntry& entry : fields.entries) {
    state = rotate_mix(state, entry.id);
    state = rotate_mix(state, entry.wire_type);
    state = rotate_mix(state, fnv1a32(entry.bytes));
  }
  state = rotate_mix(state, fields.complete ? 1u : 0u);
  return state;
}

}  // namespace wayvault
