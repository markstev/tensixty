#include "commlink.h"
#include <string.h>
#include <stdio.h>

namespace tensixty {
namespace {
unsigned char NextIndex(unsigned char index) {
  index += 1;
  return index == 128 ? 1 : index;
}
}  // namespace

PacketRingBuffer::PacketRingBuffer() {
  last_index_number_ = 0;
}

bool PacketRingBuffer::full() const {
  for (int i = 0; i < BUFFER_SIZE; ++i) {
    if (!live_indices_[i]) return false;
  }
  return true;
}

Packet* PacketRingBuffer::AllocatePacket() {
  Cleanup();
  for (int i = 0; i < BUFFER_SIZE; ++i) {
    if (!live_indices_[i]) {
      live_indices_[i] = true;
      buffer_[i].Reset();
      return &buffer_[i];
    }
  }
  return nullptr;
}

Packet* PacketRingBuffer::PopPacket() {
  for (int i = 0; i < BUFFER_SIZE; ++i) {
    if (live_indices_[i]) {
      Packet &p = buffer_[i];
      if (p.parsed() && p.index_sending() == NextIndex(last_index_number_)) {
        last_index_number_ = p.index_sending();
        live_indices_[i] = false;
        return &buffer_[i];
      }
    }
  }
  return nullptr;
}

void PacketRingBuffer::Cleanup() {
  for (int i = 0; i < BUFFER_SIZE; ++i) {
    if (live_indices_[i]) {
      Packet &p = buffer_[i];
      if (p.error()) {
        live_indices_[i] = false;
        continue;
      }
      bool in_range = false;
      unsigned char test_index = last_index_number_;
      for (int j = 0; j < BUFFER_SIZE; ++j) {
        test_index = NextIndex(test_index);
        if (test_index == p.index_sending()) {
          in_range = true;
          break;
        }
      }
      if (!in_range) {
        live_indices_[i] = false;
      }
    }
  }
}

}  // namespace tensixty
