#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace wayvault {

struct FieldCursor {
  const uint8_t* data = nullptr;
  size_t size = 0;
  size_t offset = 0;
};

struct FieldEntry {
  uint32_t id = 0;
  uint32_t wire_type = 0;
  std::vector<uint8_t> bytes;
};

struct DecodedFields {
  std::vector<FieldEntry> entries;
  std::vector<std::string> warnings;
  bool complete = true;
};

uint64_t zigzag_encode(int64_t value);
int64_t zigzag_decode(uint64_t value);
bool write_varint(uint64_t value, std::vector<uint8_t>& out);
bool read_varint(FieldCursor& cursor, uint64_t& value);
void write_field_key(uint32_t id, uint32_t wire_type, std::vector<uint8_t>& out);
bool read_field_key(FieldCursor& cursor, uint32_t& id, uint32_t& wire_type);
void write_bytes_field(uint32_t id, const std::vector<uint8_t>& bytes, std::vector<uint8_t>& out);
void write_string_field(uint32_t id, const std::string& value, std::vector<uint8_t>& out);
void write_sint_field(uint32_t id, int64_t value, std::vector<uint8_t>& out);
DecodedFields decode_fields(const uint8_t* data, size_t size, size_t max_fields = 1024);
DecodedFields decode_fields(const std::vector<uint8_t>& data, size_t max_fields = 1024);
std::string field_to_string(const FieldEntry& entry);
std::vector<FieldEntry> filter_fields(const DecodedFields& fields, uint32_t id);
uint32_t fields_hash(const DecodedFields& fields);

}  // namespace wayvault
