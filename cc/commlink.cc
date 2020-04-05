#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif 
#include "commlink.h"
#include <string.h>
#include <stdio.h>

namespace tensixty {
namespace {
unsigned char NextIndex(unsigned char index) {
  index += 1;
  return index == 128 ? 1 : index;
}

Packet* AllocatePacketFromArray(const int array_size, bool* live_indices, Packet* packet_array, int* index) {
  for (int i = 0; i < array_size ; ++i) {
    if (!live_indices[i]) {
      live_indices[i] = true;
      packet_array[i].Reset();
      *index = i;
      return &packet_array[i];
    }
  }
  return nullptr;
}

void IncrementIndex(unsigned char *index) {
  *index += 1;
  if (*index == 128) {
    *index = 1;
  }
}

const int MAX_OUTGOING_INDEX_GAP = 15;

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
  int unused_index;
  return AllocatePacketFromArray(BUFFER_SIZE, live_indices_, buffer_, &unused_index);
}

Packet* PacketRingBuffer::PopPacket() {
  for (int i = 0; i < BUFFER_SIZE; ++i) {
    if (live_indices_[i]) {
      Packet &p = buffer_[i];
      //printf("Parsed buffer_[%d].parsed = %d, index = %d, looking for: %d\n", i,
      //    p.parsed(), p.index_sending(), NextIndex(last_index_number_));
      if (p.parsed() && p.index_sending() == NextIndex(last_index_number_)) {
        last_index_number_ = p.index_sending();
        live_indices_[i] = false;
        printf("Popping\n");
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

OutgoingPacketBuffer::OutgoingPacketBuffer() {
  earliest_sent_index_ = 1;
  for (int i = 0; i < BUFFER_SIZE; ++i) {
    live_indices_[i] = false;
    pending_indices_[i] = false;
  }
}

Packet* OutgoingPacketBuffer::AllocatePacket() {
  int index;
  Packet* p = AllocatePacketFromArray(BUFFER_SIZE, live_indices_, buffer_, &index);
  if (p != nullptr) {
    pending_indices_[index] = true;
  }
  return p;
}

void OutgoingPacketBuffer::MarkSent(const unsigned char index) {
  for (int i = 0; i < BUFFER_SIZE; ++i) {
    if (live_indices_[i] && buffer_[i].index_sending() == index) {
      pending_indices_[i] = false;
    }
  }
}

void OutgoingPacketBuffer::MarkResend(const unsigned char index) {
  for (int i = 0; i < BUFFER_SIZE; ++i) {
    if (live_indices_[i] && buffer_[i].index_sending() == index) {
      printf("Found packet %d. Marking resend.\n", index);
      pending_indices_[i] = true;
    }
  }
}

void OutgoingPacketBuffer::MarkAllResend() {
  for (int i = 0; i < BUFFER_SIZE; ++i) {
    if (live_indices_[i]) {
      pending_indices_[i] = true;
      printf("Resend buffer[%d] = index %d.\n", i, buffer_[i].index_sending());
    }
  }
}

Packet* OutgoingPacketBuffer::PeekResendPacket() {
  for (int i = 0; i < BUFFER_SIZE; ++i) {
    if (live_indices_[i] && pending_indices_[i]) {
      pending_indices_[i] = false;
      return &buffer_[i];
    }
  }
  return nullptr;
}

Packet* OutgoingPacketBuffer::PeekPacket(const unsigned char index) {
  for (int i = 0; i < BUFFER_SIZE; ++i) {
    if (live_indices_[i] && buffer_[i].index_sending() == index) {
      return &buffer_[i];
    }
  }
  return nullptr;
}

Packet* OutgoingPacketBuffer::NextPacket() {
  UpdateNextIndex();
  unsigned char next_index = earliest_sent_index_;
  for (int j = 0; j < MAX_OUTGOING_INDEX_GAP; ++j) {
    for (int i = 0; i < BUFFER_SIZE; ++i) {
      if (live_indices_[i] && pending_indices_[i] &&
          buffer_[i].index_sending() == next_index) {
        return &buffer_[i];
      }
    }
    IncrementIndex(&next_index);
  }
  return nullptr;
}

bool OutgoingPacketBuffer::PrecedesIndex(unsigned char packet_index, const unsigned char sent_index) const {
  for (int i = 0; i < MAX_OUTGOING_INDEX_GAP; ++i) {
    IncrementIndex(&packet_index);
    if (packet_index == sent_index) {
      return true;
    }
  }
  return false;
}

void OutgoingPacketBuffer::RemovePacket(const unsigned char index) {
  bool removed = false;
  for (int i = 0; i < BUFFER_SIZE; ++i) {
    if (live_indices_[i] && buffer_[i].index_sending() == index) {
      live_indices_[i] = false;
      pending_indices_[i] = false;
      removed = true;
    }
  }
  if (!removed) return;
  for (int i = 0; i < BUFFER_SIZE; ++i) {
    printf("Checking for misorderings.\n");
    if (live_indices_[i] && PrecedesIndex(buffer_[i].index_sending(), index)) {
      printf("misordering found %d.\n", buffer_[i].index_sending());
      pending_indices_[i] = true;
    }
  }
  UpdateNextIndex();
}

void OutgoingPacketBuffer::UpdateNextIndex() {
  unsigned char next_index = earliest_sent_index_;
  for (int j = 0; j < MAX_OUTGOING_INDEX_GAP; ++j) {
    for (int i = 0; i < BUFFER_SIZE; ++i) {
      if (live_indices_[i] && buffer_[i].index_sending() == next_index) {
        earliest_sent_index_ = next_index;
        return;
      }
    }
    IncrementIndex(&next_index);
  }
}

Writer::Writer(const Clock &clock, SerialInterface *serial_interface, AckProvider *reader) {
  serial_interface_ = serial_interface;
  reader_ = reader;
  current_index_ = 0;
  clock_ = &clock;
  last_send_time_ = clock_->micros();
}

unsigned char Writer::NextIndex() {
  IncrementIndex(&current_index_);
  return current_index_;
}

bool Writer::AddToOutgoingQueue(const unsigned char *data,
    const unsigned int length) {
  Packet* p = buffer_.AllocatePacket();
  if (p == nullptr) return false;
  p->IncludeData(NextIndex(), data, length);
  printf("Adding packet %d\n", p->index_sending());
  return true;
}

bool Writer::Write() {
  // 1) Pick packet to write:
  // 1a) handle outgoing acks
  Ack outgoing_ack = reader_->PopOutgoingAck();
  if (outgoing_ack.index() != 0) {
    if (outgoing_ack.error()) {
      buffer_.MarkResend(outgoing_ack.index());
      printf("Mark resend %d.\n", outgoing_ack.index());
    } else {
      printf("Remove Sent Packet %d.\n", outgoing_ack.index());
      buffer_.RemovePacket(outgoing_ack.index());
    }
  }
  // 1e) resend stalled packets after time expiration.
  unsigned long now = clock_->micros();
  if (now - last_send_time_ > 100000LL) {
    buffer_.MarkAllResend();
    last_send_time_ = now;
  }
  // 1f) new packet, or empty packet with acks
  Packet *p = buffer_.NextPacket();
  if (p != nullptr) {
    if (p->ack().index() == 0) {
      p->IncludeAck(reader_->PopIncomingAck());
    }
    printf("Sending next packet: %d\n", p->index_sending());
    bool sent = SendBytes(*p);
    last_send_time_ = clock_->micros();
    return sent;
  } else if (p == nullptr) {
    Ack incoming_ack = reader_->PopIncomingAck();
    if (incoming_ack.index() != 0) {
      Packet ack_only_packet;
      ack_only_packet.IncludeAck(incoming_ack);
      printf("Sending ack-only packet.\n");
      return SendBytes(ack_only_packet);
    }
  }
  return false;
}

bool Writer::SendBytes(const Packet &p) {
  unsigned char header[7];
  unsigned char data[258];
  unsigned int data_length;
  p.Serialize(header, data, &data_length);
  for (int i = 0; i < 7; ++i) {
    serial_interface_->write(header[i]);
  }
  printf("SENDING packet %d with %d bytes acking %d error=%d \n", p.index_sending(),
      data_length, (header[2] & 0x7f), (header[2] & 0x80) == 0x80);
  digitalWrite(39, false);
  for (int i = 0; i < data_length; ++i) {
    serial_interface_->write(data[i]);
  }
  if (p.index_sending() != 0) {
    buffer_.MarkSent(p.index_sending());
  }
  return true;
}

RxTxPair::RxTxPair(const Clock &clock, SerialInterface *serial)
  : reader_(serial), writer_(clock, serial, &reader_) {}

bool RxTxPair::Transmit(const unsigned char *data, const unsigned char length) {
  return writer_.AddToOutgoingQueue(data, length);
}

const unsigned char* RxTxPair::Receive(unsigned char *length) {
  Packet* p = reader_.PopPacket();
  if (p != nullptr) {
    // Not use after free, because the data is valid until the next reader call.
    return p->data(length);
  } else {
    *length = 0;
    return nullptr;
  }
}

void RxTxPair::Tick() {
  while (reader_.Read());
  writer_.Write();
}

}  // namespace tensixty
