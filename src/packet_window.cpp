#include "wayvault/packet_window.h"

#include <algorithm>

#include "wayvault/checksum.h"

namespace wayvault {

PacketWindow::PacketWindow(size_t capacity) : capacity_(capacity == 0 ? 1 : capacity) {}

bool PacketWindow::push(PacketSpan span) {
  auto it = std::find_if(spans_.begin(), spans_.end(), [&](const PacketSpan& existing) {
    return existing.sequence == span.sequence;
  });
  if (it != spans_.end()) {
    *it = std::move(span);
    return false;
  }
  newest_ = std::max(newest_, span.sequence);
  spans_.push_back(std::move(span));
  std::stable_sort(spans_.begin(), spans_.end(), [](const PacketSpan& a, const PacketSpan& b) {
    return a.sequence < b.sequence;
  });
  while (spans_.size() > capacity_) spans_.erase(spans_.begin());
  return true;
}

std::vector<PacketSpan> PacketWindow::contiguous_from(uint32_t start) const {
  std::vector<PacketSpan> out;
  uint32_t expected = start;
  for (const PacketSpan& span : spans_) {
    if (span.sequence < expected) continue;
    if (span.sequence != expected) break;
    out.push_back(span);
    ++expected;
  }
  return out;
}

void PacketWindow::discard_before(uint32_t sequence) {
  spans_.erase(std::remove_if(spans_.begin(), spans_.end(), [&](const PacketSpan& span) {
    return span.sequence < sequence;
  }), spans_.end());
}

std::vector<uint8_t> reassemble_payloads(const std::vector<PacketSpan>& spans) {
  std::vector<uint8_t> out;
  for (const PacketSpan& span : spans) {
    out.insert(out.end(), span.payload.begin(), span.payload.end());
  }
  return out;
}

uint32_t packet_window_hash(const PacketWindow& window, uint32_t start) {
  uint32_t state = rotate_mix(window.newest_sequence(), static_cast<uint32_t>(window.size()));
  for (const PacketSpan& span : window.contiguous_from(start)) {
    state = rotate_mix(state, span.sequence);
    state = rotate_mix(state, fnv1a32(span.payload));
  }
  return state;
}

}  // namespace wayvault
