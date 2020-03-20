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

  header_first_checksum_ = 0;
  header_second_checksum_ = 0;
  data_first_checksum_ = 0;
  data_second_checksum_ = 0;
}

ParseStatus Packet::ParseChar(const unsigned char c) {
  ParseStatus status;
  if (header_next_byte_index_ < NUM_HEADER_BYTES) {
    status = ParseHeaderChar(c);
  } else {
    status = ParseDataChar(c);
  }
  switch (status) {
    case PARSED:
      parsed_ = true;
      break;
    case HEADER_ERROR:
      error_ = true;
      break;
    case DATA_ERROR:
      error_ = true;
      break;
    case INCOMPLETE:
      break;
  }
  return status;
}

namespace {
void UpdateChecksum(const unsigned char c, unsigned char *first_sum, unsigned char *second_sum) {
  *first_sum += c;
  *second_sum += *first_sum;
}
}

ParseStatus Packet::ParseHeaderChar(const unsigned char c) {
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
  if (error) {
    Reset();
    return HEADER_ERROR;
  }
  UpdateChecksum(c, &header_first_checksum_, &header_second_checksum_);
  ++header_next_byte_index_;
  return INCOMPLETE;
}

ParseStatus Packet::ParseDataChar(const unsigned char c) {
  if (data_next_byte_index_ < static_cast<const unsigned int>(data_length_)) {
    data_[data_next_byte_index_] = c;
    UpdateChecksum(c, &data_first_checksum_, &data_second_checksum_);
  } else if (data_next_byte_index_ == static_cast<const unsigned int>(data_length_)) {
    if (c != data_first_checksum_) {
      printf("Checksum expected %d != actual %d\n", data_first_checksum_, c);
      Reset();
      return DATA_ERROR;
    }
  } else if (data_next_byte_index_ == static_cast<const unsigned int>(data_length_) + 1) {
    if (c != data_second_checksum_) {
      printf("Checksum expected %d != actual %d\n", data_second_checksum_, c);
      Reset();
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

void Packet::Serialize(unsigned char *header, unsigned char *data, unsigned char *data_bytes) {
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
