#include "commlink.h"
#include <string.h>
#include <stdio.h>

namespace tensixty {
namespace {

const long long kResendPeriod = 100000LL;
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
  if (*index >= 128) {
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
        //printf("Popping\n");
        return &buffer_[i];
      }
    }
  }
  return nullptr;
}

void PacketRingBuffer::Clear() {
  for (int i = 0; i < BUFFER_SIZE; ++i) {
    live_indices_[i] = false;
  }
  last_index_number_ = 0;
}

bool PacketRingBuffer::InRange(const unsigned char index_sending) {
  unsigned char test_index = last_index_number_;
  for (int j = 0; j < BUFFER_SIZE; ++j) {
    test_index = NextIndex(test_index);
    if (test_index == index_sending) {
      return true;
    }
  }
  return false;
}

void PacketRingBuffer::Cleanup() {
  // Note that this is only called when allocating a packet, so there shouldn't
  // be any incomplete packets in the list.
  for (int i = 0; i < BUFFER_SIZE; ++i) {
    Packet &p = buffer_[i];
    if (!live_indices_[i]) continue;
    if (p.error()) {
      live_indices_[i] = false;
      continue;
    }
    if (!InRange(p.index_sending())) {
      live_indices_[i] = false;
      continue;
    }
    // Remove duplicates.
    for (int j = i + 1; j < BUFFER_SIZE; ++j) {
      if (live_indices_[j] && p.index_sending() == buffer_[j].index_sending()) {
        printf("Cleanup Dropping %d\n", p.index_sending());
        live_indices_[j] = false;
        break;
      }
    }
  }
}

Reader::Reader(const int name, SerialInterface *serial)
  : name_(name) {
  serial_ = serial;
  current_packet_ = nullptr;
  sequence_started_ = false;
}

bool Reader::Read() {
  //printf("Starting read.\n");
  if (!serial_->available()) return false;
  //printf("Got bytes.\n");
  if (incoming_ack_.index() != 0) {
    printf("%d: Reader waiting on incoming ack.\n", name_);
    return false;
  }
  if (outgoing_ack_.index() != 0) {
    printf("%d: Reader waiting on outgoing ack.\n", name_);
    return false;
  }
  //printf("Old acks flushed okay.\n");
  if (current_packet_ == nullptr) {
    current_packet_ = buffer_.AllocatePacket();
  }
  //printf("Allocated packet.\n");
  // No buffer space left.
  if (current_packet_ == nullptr) {
    printf("%d: No buffer space left.\n", name_);
    return false;
  }

  const ParseStatus status =
    current_packet_->ParseChar(serial_->read());

  if (status == INCOMPLETE) return true;
  if (status == HEADER_ERROR) {
    current_packet_ = nullptr;
    return true;
  }
  outgoing_ack_ = current_packet_->ack();
  printf("%d: CC Outgoing ack: %d\n", name_, outgoing_ack_.Serialize());
  if (!sequence_started_ && !current_packet_->start_sequence()) {
    current_packet_ = nullptr;
    buffer_.Clear();
    printf("%d: Got bytes but not initialized yet.\n", name_);
  } else if (current_packet_->start_sequence()) {
    incoming_ack_.AckStartSequence();
    buffer_.Clear();
    sequence_started_ = true;
    printf("%d: Reader sequence started.\n", name_);
  } else {
    if (status != PARSED) {
      incoming_ack_.Parse(true, current_packet_->index_sending());
    } else if (!buffer_.InRange(current_packet_->index_sending())) {
      // Acks out of order packets. We already received these, but the
      // ack reply must have been corrupted.
      incoming_ack_.Parse(/*error=*/ true, current_packet_->index_sending());
    }
    // Can't ack if parsed, since it may be out of order. We'll ack on pop().
  }
  current_packet_ = nullptr;
  return true;
}

Packet* Reader::PopPacket() {
  Packet *packet = buffer_.PopPacket();
  if (packet != nullptr) {
    // TODO: Problem is that if we've already popped, we'll never ack a retry.
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

OutgoingPacketBuffer::OutgoingPacketBuffer(int name) {
  earliest_sent_index_ = 0x80;
  for (int i = 0; i < BUFFER_SIZE; ++i) {
    live_indices_[i] = false;
    pending_indices_[i] = false;
  }
  sequence_started_ = false;
  name_ = name;
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
    if (live_indices_[i] && (buffer_[i].index_sending() == index)) {
      pending_indices_[i] = false;
    }
  }
}

void OutgoingPacketBuffer::MarkResend(const unsigned char index) {
  for (int i = 0; i < BUFFER_SIZE; ++i) {
    if (live_indices_[i] && (buffer_[i].index_sending() == index)) {
      printf("%d: Found packet %d. Marking resend.\n", name_, index);
      pending_indices_[i] = true;
    }
  }
}

void OutgoingPacketBuffer::MarkAllResend() {
  for (int i = 0; i < BUFFER_SIZE; ++i) {
    if (live_indices_[i]) {
      pending_indices_[i] = true;
      printf("%d: Resend buffer[%d] = index %d.\n", name_, i, buffer_[i].index_sending());
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
  printf("%d: Removed packet %d = %d\n", name_, index, removed);
  if (!removed) return;
  for (int i = 0; i < BUFFER_SIZE; ++i) {
    if (live_indices_[i] && PrecedesIndex(buffer_[i].index_sending(), index)) {
      printf("%d: misordering found %d.\n", name_, buffer_[i].index_sending());
      pending_indices_[i] = true;
    }
  }
  UpdateNextIndex();
}

void OutgoingPacketBuffer::MarkSequenceStarted() {
  sequence_started_ = true;
  earliest_sent_index_ = 1;
}

void OutgoingPacketBuffer::UpdateNextIndex() {
  if (!sequence_started_) {
    earliest_sent_index_ = 0x80;
  }
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

Writer::Writer(const int name, const Clock &clock, SerialInterface *serial_interface, AckProvider *reader)
  : buffer_(name), name_(name) {
  serial_interface_ = serial_interface;
  reader_ = reader;
  current_index_ = 0;
  clock_ = &clock;
  last_send_time_ = clock_->micros();
  sequence_started_ = false;
  {
    // Send the initialization packet.
    Packet* p = buffer_.AllocatePacket();
    p->IncludeData(0x80, nullptr, 0);
  }
}

unsigned char Writer::NextIndex() {
  IncrementIndex(&current_index_);
  return current_index_;
}

bool Writer::AddToOutgoingQueue(const unsigned char *data,
    const unsigned int length) {
  if (!sequence_started_) return false;
  Packet* p = buffer_.AllocatePacket();
  if (p == nullptr) return false;
  p->IncludeData(NextIndex(), data, length);
  printf("%d: Adding packet %d\n", name_, p->index_sending());
  return true;
}

bool Writer::Write() {
  // 1) Pick packet to write:
  // 1a) handle outgoing acks
  Ack outgoing_ack = reader_->PopOutgoingAck();
  if (outgoing_ack.index() != 0) {
    printf("%d: Got ack for %d.\n", name_, outgoing_ack.index());
    if (outgoing_ack.error()) {
      buffer_.MarkResend(outgoing_ack.index());
      printf("%d: Mark resend %d.\n", name_, outgoing_ack.index());
    } else {
      printf("%d: Remove Sent Packet %d.\n", name_, outgoing_ack.index());
      buffer_.RemovePacket(outgoing_ack.index());
    }
  } else if (outgoing_ack.is_start_sequence_ack()) {
    printf("%d: Writer sequence started.\n", name_);
    sequence_started_ = true;
    buffer_.RemovePacket(0);
    buffer_.RemovePacket(0x80);
    buffer_.MarkSequenceStarted();
  } else {
    //printf("No acks.\n");
  }
  // 1e) resend stalled packets after time expiration.
  unsigned long now = clock_->micros();
  if (now - last_send_time_ > kResendPeriod) {
    buffer_.MarkAllResend();
    last_send_time_ = now;
  }
  // 1f) new packet, or empty packet with acks
  Packet *p = buffer_.NextPacket();
  if (p != nullptr) {
    printf("%d: Sending new packet from buffer\n", name_);
    if (p->ack().index() == 0) {
      p->IncludeAck(reader_->PopIncomingAck());
    }
    printf("%d: Sending next packet: %d\n", name_, p->index_sending());
    bool sent = SendBytes(*p);
    last_send_time_ = clock_->micros();
    return sent;
  } else if (p == nullptr) {
    //printf("No packets from buffer\n");
    Ack incoming_ack = reader_->PopIncomingAck();
    if (incoming_ack.index() != 0 || incoming_ack.is_start_sequence_ack()) {
      Packet ack_only_packet;
      ack_only_packet.IncludeAck(incoming_ack);
      printf("%d: Sending ack-only packet.\n", name_);
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
  printf("%d: SENDING packet %d with %d bytes acking %d error=%d. Writer initialized=%d \n", name_, p.index_sending(),
      data_length, (header[2] & 0x7f), (header[2] & 0x80) == 0x80, sequence_started_);
  for (int i = 0; i < data_length; ++i) {
    serial_interface_->write(data[i]);
  }
  if (p.index_sending() != 0 || p.start_sequence()) {
    buffer_.MarkSent(p.index_sending());
  }
  return true;
}

RxTxPair::RxTxPair(const int name, const Clock &clock, SerialInterface *serial)
  : reader_(name, serial), writer_(name, clock, serial, &reader_), name_(name) {}

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
