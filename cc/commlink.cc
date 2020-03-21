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

Reader::Reader(SerialInterface *serial) {
  serial_ = serial;
  current_packet_ = nullptr;
}

bool Reader::Read() {
  if (!serial_->available()) return false;
  if (incoming_ack_.index() != 0) return false;
  if (outgoing_ack_.index() != 0) return false;
  if (current_packet_ == nullptr) {
    current_packet_ = buffer_.AllocatePacket();
  }
  // No buffer space left.
  if (current_packet_ == nullptr) return false;

  const ParseStatus status =
    current_packet_->ParseChar(serial_->read());

  if (status != INCOMPLETE) {
    if (status != HEADER_ERROR) {
      outgoing_ack_ = current_packet_->ack();
      if (status != PARSED) {
        incoming_ack_.Parse(true, current_packet_->index_sending());
        // Can't ack if parsed, since it may be out of order.
      }
    }
    current_packet_ = nullptr;
  }
  return true;
}

Packet* Reader::PopPacket() {
  Packet *packet = buffer_.PopPacket();
  if (packet != nullptr) {
    incoming_ack_.Parse(false, packet->index_sending());
  }
  return packet;
}

Ack Reader::PopIncomingAck() {
  Ack ack = incoming_ack_;
  incoming_ack_.Parse(0x00);
  return ack;
}

Ack Reader::PopOutgoingAck() {
  Ack ack = outgoing_ack_;
  outgoing_ack_.Parse(0x00);
  return ack;
}

}  // namespace tensixty
