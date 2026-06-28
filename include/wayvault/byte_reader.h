#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "wayvault/error.h"

namespace wayvault {

class ByteReader {
 public:
  ByteReader(const uint8_t* data, size_t size);

  size_t position() const { return pos_; }
  size_t remaining() const;
  size_t size() const { return size_; }
  bool empty() const { return remaining() == 0; }

  void seek(size_t pos);
  void skip(size_t count);

  uint8_t read_u8();
  uint16_t read_u16();
  uint32_t read_u32();
  uint64_t read_u64();
  std::vector<uint8_t> read_bytes(size_t count);
  std::string read_string(size_t count);

  const uint8_t* pointer_at(size_t pos, size_t count) const;
  ByteReader subreader(size_t pos, size_t count) const;

 private:
  void require(size_t count) const;

  const uint8_t* data_;
  size_t size_;
  size_t pos_;
};

bool checked_add(size_t a, size_t b, size_t* out);
bool checked_mul(size_t a, size_t b, size_t* out);

}  // namespace wayvault
