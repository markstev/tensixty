#include "packet.h"
#include "serial_interface.h"

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
  Reader(SerialInterface *arduino);
  // Returns true if anything was read and the reader can keep reading.
  // Pending acks, lack of data, or a full read buffer will cause this to return false.
  bool Read();
  // Returns a finished packet. Null if there are no packets.
  Packet* PopPacket();
  // Returns incoming and outgoing acks.
  Ack PopIncomingAck();
  Ack PopOutgoingAck();

 private:
  SerialInterface *serial_;
  // Packet under construction. Points to an object stored in buffer_.
  Packet *current_packet_;
  PacketRingBuffer buffer_;
  Ack incoming_ack_;
  Ack outgoing_ack_;
};

class Writer {
 public:
  Writer(SerialInterface *arduino);
  void AddToOutgoingQueue(const Packet &packet);
  void Write(const Ack &incoming_ack, const Ack &outgoing_ack);
 private:
  SerialInterface *arduino_;
};

}  // namespace tensixty
