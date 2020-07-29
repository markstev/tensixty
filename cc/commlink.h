#ifndef TENSIXTY_COMMLINK_H_
#define TENSIXTY_COMMLINK_H_

#include "packet.h"
#include "serial_interface.h"
#include "clock_interface.h"

namespace tensixty {

const unsigned int BUFFER_SIZE = 4;

class PacketRingBuffer {
 public:
  PacketRingBuffer();
  // Allocates a packet from the buffer.
  Packet* AllocatePacket();
  // Returns true if there is no space left.
  bool full() const;
  // Pops the next packet, in order.
  Packet* PopPacket();
  // Deletes all entries in the buffer.
  void Clear();
  // Returns true if the given index is valid as an incoming packet index.
  bool InRange(unsigned char index_sending);
 private:
  // Clears any packets left in an erroneous state as well as any packets too far
  // from the last_index_number_.
  void Cleanup();

  Packet buffer_[BUFFER_SIZE];
  bool live_indices_[BUFFER_SIZE];
  unsigned char last_index_number_;
};

class OutgoingPacketBuffer {
 public:
  explicit OutgoingPacketBuffer(int name);
  // Allocates a packet from the buffer.
  Packet* AllocatePacket();

  // Returns packets that need to be resent, if any.
  Packet* PeekResendPacket();
  // Finds the packet with the given index. If there is none indicated, returns
  // the next packet to be popped. Does not remove the packet, as pop() does.
  Packet* PeekPacket(unsigned char index);
  Packet* NextPacket();
  void RemovePacket(unsigned char index);
  void MarkSent(unsigned char index);
  void MarkResend(unsigned char index);
  void MarkAllResend();
  void MarkSequenceStarted();
 private:
  // Returns true if packet_index precedes sent_index.
  bool PrecedesIndex(unsigned char packet_index, unsigned char sent_index) const;
  void UpdateNextIndex();
  Packet buffer_[BUFFER_SIZE];
  bool live_indices_[BUFFER_SIZE];
  bool pending_indices_[BUFFER_SIZE];
  // Makes it easier to handle indices wrapping around.
  unsigned char earliest_sent_index_;
  bool sequence_started_;
  int name_;
};

class AckProvider {
 public:
  // Returns incoming and outgoing acks.
  virtual Ack PopIncomingAck() = 0;
  virtual Ack PopOutgoingAck() = 0;
};

class Reader : public AckProvider {
 public:
  Reader(int name, SerialInterface *arduino);
  // Returns true if anything was read and the reader can keep reading.
  // Pending acks, lack of data, or a full read buffer will cause this to return false.
  bool Read();
  // Returns a finished packet. Null if there are no packets.
  Packet* PopPacket();
  // Returns incoming and outgoing acks.
  Ack PopIncomingAck() override;
  Ack PopOutgoingAck() override;
  bool Initialized() const { return sequence_started_; };

 private:
  SerialInterface *serial_;
  // Packet under construction. Points to an object stored in buffer_.
  Packet *current_packet_;
  PacketRingBuffer buffer_;
  Ack incoming_ack_;
  Ack outgoing_ack_;
  bool sequence_started_;
  const int name_;
};

class Writer {
 public:
  Writer(int name, const Clock &clock, SerialInterface *arduino, AckProvider *reader);
  // Returns false if we can't accept the packet.
  bool AddToOutgoingQueue(const unsigned char *data, const unsigned int length);
  bool Write();
  bool Initialized() const { return sequence_started_; };
 private:
  const Clock* clock_;
  unsigned char NextIndex();
  // Returns true if bytes are sent.
  bool SendBytes(const Packet &p);

  SerialInterface *serial_interface_;
  OutgoingPacketBuffer buffer_;
  AckProvider *reader_;
  unsigned char current_index_;
  unsigned long last_send_time_;
  bool sequence_started_;
  const int name_;
};

class RxTxPair {
 public:
  RxTxPair(int name, const Clock &clock, SerialInterface *serial);
  bool Transmit(const unsigned char *data, const unsigned char length);
  const unsigned char* Receive(unsigned char *length);
  void Tick();
  bool Initialized() const { return reader_.Initialized() && writer_.Initialized(); }

 private:
  Reader reader_;
  Writer writer_;
};

}  // namespace tensixty

#endif  // TENSIXTY_COMMLINK_H_
