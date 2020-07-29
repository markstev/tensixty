#ifndef TENSIXTY_PACKET_H_
#define TENSIXTY_PACKET_H_

namespace tensixty {

enum ParseStatus {
  PARSED,
  HEADER_ERROR,
  DATA_ERROR,  // Implies header is valid.
  INCOMPLETE
};

class Ack {
 public:
   Ack();
   explicit Ack(const unsigned char index_and_error);
   void Parse(const unsigned char index_and_error);
   void Parse(bool error, const unsigned char index);
   void AckStartSequence();
   const unsigned char Serialize() const;
   unsigned char index() const;
   bool error() const;
   bool ok() const;
   bool is_start_sequence_ack() const;
   Ack& operator=(const Ack &other) noexcept;

 private:
   unsigned char index_and_error_;
};

class Packet {
 public:
  Packet();
  void Reset();

  ParseStatus ParseChar(const unsigned char c);

  // Accessors
  const Ack& ack() const { return ack_; }
  unsigned char index_sending() const { return index_sending_; }
  const unsigned char *data(unsigned char *length) const;
  // True if the message is completely parsed.
  bool parsed() const { return parsed_; }
  // True if the message encountered an error while parsing.
  bool error() const { return error_; }
  // True if the message indicates a new connection.
  bool start_sequence() const { return index_sending_ == 0x80; }

  // Builder
  void IncludeAck(const Ack &ack);
  void IncludeData(const unsigned char index, const unsigned char *data, unsigned int data_length);

  // Copys the contents out to another pair of arrays, header and data_bytes. Header must be at least 7 bytes long,
  // and data must be at least 258 bytes long.
  void Serialize(unsigned char *header, unsigned char *data, unsigned int *data_bytes) const;

 private:
  ParseStatus ParseCharInternal(const unsigned char c);
  ParseStatus ParseHeaderChar(const unsigned char c);
  ParseStatus ParseDataChar(const unsigned char c);
  ParseStatus ReProcessPacket();

  Ack ack_;
  unsigned char index_sending_ = 0;
  unsigned char data_[256];
  unsigned char data_length_ = 0;
  bool parsed_, error_;

  // Partial data while parsing.
  unsigned int header_next_byte_index_;
  unsigned int data_next_byte_index_;

  unsigned char header_first_checksum_;
  unsigned char header_second_checksum_;

  unsigned char data_first_checksum_;
  unsigned char data_second_checksum_;
};

}  // namespace tensixty

#endif  // TENSIXTY_PACKET_H_
