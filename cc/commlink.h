#include "packet.h"

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
 private:
  // Clears any packets left in an erroneous state as well as any packets too far
  // from the last_index_number_.
  void Cleanup();

  Packet buffer_[BUFFER_SIZE];
  bool live_indices_[BUFFER_SIZE];
  unsigned char last_index_number_;
};

class Reader {
 public:
  void Read();
  Ack PopIncomingAck();
  Ack PopOutgoingAck();

 private:
  PacketRingBuffer buffer_;
  Ack incoming_ack_;
  Ack outgoing_ack_;
};

class Writer {
 public:
  void AddToOutgoingQueue(const Packet &packet);
  void Write(const Ack &incoming_ack, const Ack &outgoing_ack);
 private:
};

}  // namespace tensixty
