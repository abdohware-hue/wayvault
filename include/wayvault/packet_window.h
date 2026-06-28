#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace wayvault {

struct PacketSpan {
  uint32_t sequence = 0;
  std::vector<uint8_t> payload;
};

class PacketWindow {
 public:
  explicit PacketWindow(size_t capacity);
  bool push(PacketSpan span);
  std::vector<PacketSpan> contiguous_from(uint32_t start) const;
  void discard_before(uint32_t sequence);
  size_t size() const { return spans_.size(); }
  uint32_t newest_sequence() const { return newest_; }

 private:
  size_t capacity_;
  uint32_t newest_ = 0;
  std::vector<PacketSpan> spans_;
};

uint32_t packet_window_hash(const PacketWindow& window, uint32_t start);
std::vector<uint8_t> reassemble_payloads(const std::vector<PacketSpan>& spans);

}  // namespace wayvault
