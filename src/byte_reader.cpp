#include "wayvault/byte_reader.h"

#include <limits>

namespace wayvault {

ByteReader::ByteReader(const uint8_t* data, size_t size) : data_(data), size_(size), pos_(0) {}

size_t ByteReader::remaining() const {
  return pos_ <= size_ ? size_ - pos_ : 0;
}

void ByteReader::seek(size_t pos) {
  if (pos > size_) {
    throw ParseError("seek past end of buffer");
  }
  pos_ = pos;
}

void ByteReader::skip(size_t count) {
  require(count);
  pos_ += count;
}

void ByteReader::require(size_t count) const {
  if (count > remaining()) {
    throw ParseError("truncated input");
  }
}

uint8_t ByteReader::read_u8() {
  require(1);
  return data_[pos_++];
}

uint16_t ByteReader::read_u16() {
  require(2);
  uint16_t v = static_cast<uint16_t>(data_[pos_]) |
               (static_cast<uint16_t>(data_[pos_ + 1]) << 8);
  pos_ += 2;
  return v;
}

uint32_t ByteReader::read_u32() {
  require(4);
  uint32_t v = static_cast<uint32_t>(data_[pos_]) |
               (static_cast<uint32_t>(data_[pos_ + 1]) << 8) |
               (static_cast<uint32_t>(data_[pos_ + 2]) << 16) |
               (static_cast<uint32_t>(data_[pos_ + 3]) << 24);
  pos_ += 4;
  return v;
}

uint64_t ByteReader::read_u64() {
  uint64_t low = read_u32();
  uint64_t high = read_u32();
  return low | (high << 32);
}

std::vector<uint8_t> ByteReader::read_bytes(size_t count) {
  require(count);
  std::vector<uint8_t> out(data_ + pos_, data_ + pos_ + count);
  pos_ += count;
  return out;
}

std::string ByteReader::read_string(size_t count) {
  require(count);
  std::string out(reinterpret_cast<const char*>(data_ + pos_), count);
  pos_ += count;
  return out;
}

const uint8_t* ByteReader::pointer_at(size_t pos, size_t count) const {
  size_t end = 0;
  if (!checked_add(pos, count, &end) || end > size_) {
    throw ParseError("slice outside buffer");
  }
  return data_ + pos;
}

ByteReader ByteReader::subreader(size_t pos, size_t count) const {
  const uint8_t* ptr = pointer_at(pos, count);
  return ByteReader(ptr, count);
}

bool checked_add(size_t a, size_t b, size_t* out) {
  if (b > std::numeric_limits<size_t>::max() - a) {
    return false;
  }
  *out = a + b;
  return true;
}

bool checked_mul(size_t a, size_t b, size_t* out) {
  if (a != 0 && b > std::numeric_limits<size_t>::max() / a) {
    return false;
  }
  *out = a * b;
  return true;
}

}  // namespace wayvault
