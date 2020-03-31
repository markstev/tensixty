#include "packet.h"
#include <string.h>
#include <stdio.h>

namespace tensixty {
namespace {
const int NUM_HEADER_BYTES = 7;
}  // namespace

Ack::Ack() {
  index_and_error_ = 0;
}

Ack::Ack(const unsigned char index_and_error) {
  Parse(index_and_error);
}

void Ack::Parse(const unsigned char index_and_error) {
  index_and_error_ = index_and_error;
}

void Ack::Parse(bool error, const unsigned char index) {
  index_and_error_ = (error ? 0x80 : 0x00) | (0x7f & index);
}

unsigned char Ack::index() const {
  return index_and_error_ & 0x7f;
}

bool Ack::error() const {
  return 0x80 == (index_and_error_ & 0x80);
}

bool Ack::ok() const {
  return !error();
}

const unsigned char Ack::Serialize() const {
  return index_and_error_;
}

Ack& Ack::operator=(const Ack &other) noexcept {
  index_and_error_ = other.Serialize();
  return *this;
}

Packet::Packet() {
  Reset();
}

void Packet::Reset() {
  parsed_ = false;
  error_ = false;
  header_next_byte_index_ = 0;
  data_next_byte_index_ = 0;
  ack_.Parse(0x00);

  header_first_checksum_ = 0;
  header_second_checksum_ = 0;
  data_first_checksum_ = 0;
  data_second_checksum_ = 0;
}

ParseStatus Packet::ParseChar(const unsigned char c) {
  ParseStatus status = ParseCharInternal(c);
  if (status == HEADER_ERROR || status == DATA_ERROR) {
    printf("Reprocess %d for char %d, status %d\n", status, c, status);
    const ParseStatus adjusted_status = ReProcessPacket();
    if (adjusted_status == INCOMPLETE || adjusted_status == PARSED) {
      printf("Reprocess looks good 2\n");
      status = adjusted_status;
    }
  }
  switch (status) {
    case PARSED:
      parsed_ = true;
      printf("Parsed! Packet %d with %d bytes\n", index_sending_, data_length_);
      break;
    case HEADER_ERROR:
      Reset();
      error_ = true;
      break;
    case DATA_ERROR:
      Reset();
      error_ = true;
      break;
    case INCOMPLETE:
      break;
  }
  return status;
}

ParseStatus Packet::ReProcessPacket() {
  unsigned char byte_stream[7 + 258];
  unsigned int num_bytes;
  Serialize(byte_stream, byte_stream + 7 * sizeof(unsigned char), &num_bytes);
  if (header_next_byte_index_ < NUM_HEADER_BYTES) {
    num_bytes = header_next_byte_index_;
  } else {
    num_bytes = data_next_byte_index_;
  }
  printf("ReProcess: Num bytes = %d\n", num_bytes);
  ParseStatus status = HEADER_ERROR;
  for (unsigned int start = 1; start < num_bytes; ++start) {
    Reset();
    printf("ReProcess from start = %d\n", start);
    int i = start;
    for (; i < num_bytes; ++i) {
      status = ParseCharInternal(byte_stream[i]);
      printf("ReProcess %d -> %d\n", i , status);
      if (status == HEADER_ERROR || status == DATA_ERROR) {
        break;
      }
    }
    if (status == INCOMPLETE || status == PARSED) {
      printf("ReProcess looks good!\n");
      return status;
    }
  }
  return status;
}


ParseStatus Packet::ParseCharInternal(const unsigned char c) {
  if (header_next_byte_index_ < NUM_HEADER_BYTES) {
    return ParseHeaderChar(c);
  } else {
    return ParseDataChar(c);
  }
}

namespace {
void UpdateChecksum(const unsigned char c, unsigned char *first_sum, unsigned char *second_sum) {
  *first_sum += c;
  *second_sum += *first_sum;
}
}

ParseStatus Packet::ParseHeaderChar(const unsigned char c) {
  printf("Parsing %d, header_index = %d\n", c, header_next_byte_index_);
  bool error = false;
  switch (header_next_byte_index_) {
    case 0: {
      if (c != 10) error = true;
      break;
    }
    case 1: {
      if (c != 60) error = true;
      break;
    }
    case 2: {
      ack_.Parse(c);
      break;
    }
    case 3: {
      index_sending_ = c;
      break;
    }
    case 4: {
      data_length_ = c;
      break;
    }
    case 5: {
      if (c != header_first_checksum_) {
        printf("Header mismatch %d expected\n", header_first_checksum_);
        error = true;
      } else {
        ++header_next_byte_index_;
        return INCOMPLETE;
      }
      break;
    }
    case 6: {
      if (c != header_second_checksum_) {
        error = true;
      } else {
        ++header_next_byte_index_;
        return INCOMPLETE;
      }
      break;
    }
  }
  ++header_next_byte_index_;
  if (error) {
    return HEADER_ERROR;
  }
  UpdateChecksum(c, &header_first_checksum_, &header_second_checksum_);
  return INCOMPLETE;
}

ParseStatus Packet::ParseDataChar(const unsigned char c) {
  if (data_next_byte_index_ < static_cast<const unsigned int>(data_length_)) {
    data_[data_next_byte_index_] = c;
    UpdateChecksum(c, &data_first_checksum_, &data_second_checksum_);
  } else if (data_next_byte_index_ == static_cast<const unsigned int>(data_length_)) {
    if (c != data_first_checksum_) {
      printf("Checksum expected %d != actual %d\n", data_first_checksum_, c);
      return DATA_ERROR;
    }
  } else if (data_next_byte_index_ == static_cast<const unsigned int>(data_length_) + 1) {
    if (c != data_second_checksum_) {
      printf("Checksum expected %d != actual %d\n", data_second_checksum_, c);
      return DATA_ERROR;
    }
    return PARSED;
  }
  ++data_next_byte_index_;
  return INCOMPLETE;
}

const unsigned char* Packet::data(unsigned char *length) const {
  *length = data_length_;
  return data_;
}

void Packet::IncludeAck(const Ack &ack) {
  ack_ = ack;
}

void Packet::IncludeData(const unsigned char index, const unsigned char *data, const unsigned int data_length) {
  index_sending_ = index;
  data_length_ = data_length;
  memcpy(data_, data, data_length * sizeof(unsigned char));
}

void Packet::Serialize(unsigned char *header, unsigned char *data, unsigned int *data_bytes) const {
  header[0] = 10;
  header[1] = 60;
  header[2] = ack_.Serialize();
  header[3] = index_sending_;
  header[4] = data_length_;
  unsigned char ck_0 = 0;
  unsigned char ck_1 = 0;
  for (int i = 0; i < 5; ++i) {
    UpdateChecksum(header[i], &ck_0, &ck_1);
  }
  header[5] = ck_0;
  header[6] = ck_1;

  ck_0 = 0;
  ck_1 = 0;
  *data_bytes = data_length_ + 2;
  for (int i = 0; i < data_length_; ++i) {
    data[i] = data_[i];
    UpdateChecksum(data[i], &ck_0, &ck_1);
  }
  data[data_length_] = ck_0;
  data[data_length_ + 1] = ck_1;
}

}  // namespace tensixty
