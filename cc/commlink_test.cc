// Using https://github.com/google/googletest

#include <gtest/gtest.h>
#include "commlink.h"
#include "arduino_simulator.h"
#include "serial_interface.h"

namespace tensixty {
namespace {

void FillPacket(const Ack &ack, const unsigned char index, Packet *packet) {
  Packet pt;
  pt.IncludeAck(ack);
  unsigned char data[1];
  data[0] = index + 1;
  pt.IncludeData(index, data, 1);
  unsigned char header[7];
  unsigned char contents[3];
  unsigned int length;
  pt.Serialize(header, contents, &length);
  for (int i = 0; i < 7; ++i) {
    packet->ParseChar(header[i]);
  }
  for (int i = 0; i < length; ++i) {
    packet->ParseChar(contents[i]);
  }
  EXPECT_TRUE(packet->parsed());
}

TEST(PacketRingBufferTest, AllocateMaxPackets) {
  PacketRingBuffer rb;
  for (int i = 0; i < 4; ++i) {
    EXPECT_FALSE(rb.full());
    Packet *p = rb.AllocatePacket();
    ASSERT_NE(p, nullptr);
    FillPacket(Ack(0x70), i + 1, p);
  }
  EXPECT_TRUE(rb.full());
}

TEST(PacketRingBufferTest, PopNextSeveral) {
  PacketRingBuffer rb;
  for (int i = 0; i < 4; ++i) {
    EXPECT_FALSE(rb.full());
    Packet *p = rb.AllocatePacket();
    ASSERT_NE(p, nullptr);
    FillPacket(Ack(0x70), i + 1, p);
  }
  EXPECT_TRUE(rb.full());
  Packet *p = rb.PopPacket();
  for (int i = 1; i < 2; ++i) {
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->index_sending(), i);
    unsigned char length;
    EXPECT_EQ(p->data(&length)[0], i + 1);
    EXPECT_EQ(length, 1);
  }
}

unsigned char ToIndex(const int i) {
  return i % 127 + 1;
}

TEST(PacketRingBufferTest, DropOutOfOrderTooLow) {
  PacketRingBuffer rb;
  for (int i = 0; i < 30; ++i) {
    EXPECT_FALSE(rb.full());
    Packet *p = rb.AllocatePacket();
    ASSERT_NE(p, nullptr);
    FillPacket(Ack(0x70), ToIndex(i), p);

    Packet *popped = rb.PopPacket();
    ASSERT_NE(popped, nullptr);
    EXPECT_EQ(popped->index_sending(), ToIndex(i));
    unsigned char length;
    EXPECT_EQ(popped->data(&length)[0], ToIndex(i) + 1);
    EXPECT_EQ(length, 1);
  }
  // Now, try some lower indices. Should get dropped.
  for (int i = 20; i < 30; ++i) {
    EXPECT_FALSE(rb.full());
    Packet *p = rb.AllocatePacket();
    ASSERT_NE(p, nullptr);
    FillPacket(Ack(0x70), ToIndex(i), p);
  }
  EXPECT_FALSE(rb.full());
}

TEST(PacketRingBufferTest, DropOutOfOrderTooHigh) {
  PacketRingBuffer rb;
  for (int i = 0; i < 4; ++i) {
    EXPECT_FALSE(rb.full());
    Packet *p = rb.AllocatePacket();
    ASSERT_NE(p, nullptr);
    unsigned char index = i + 1;
    if (i == 3) {
      index = 13;
    }
    FillPacket(Ack(0x70), index, p);
  }
  Packet *p = rb.AllocatePacket();
  ASSERT_NE(p, nullptr);
}

TEST(PacketRingBufferTest, PushAndPop) {
  PacketRingBuffer rb;
  for (int i = 0; i < 300; ++i) {
    EXPECT_FALSE(rb.full());
    Packet *p = rb.AllocatePacket();
    ASSERT_NE(p, nullptr);
    FillPacket(Ack(0x70), ToIndex(i), p);

    Packet *popped = rb.PopPacket();
    ASSERT_NE(popped, nullptr);
    EXPECT_EQ(popped->index_sending(), ToIndex(i));
    unsigned char length;
    EXPECT_EQ(popped->data(&length)[0], ToIndex(i) + 1);
    EXPECT_EQ(length, 1);
  }
}

void WritePacketError(const Ack &ack, const unsigned char index,
    const unsigned char header_error_index,
    const unsigned char data_error_index, SerialInterface *s) {
  Packet pt;
  pt.IncludeAck(ack);
  unsigned char data[1];
  data[0] = index + 1;
  pt.IncludeData(index, data, 1);
  unsigned char header[7];
  unsigned char contents[3];
  unsigned int length;
  pt.Serialize(header, contents, &length);
  for (int i = 0; i < 7; ++i) {
    if (i == header_error_index) {
      s->write(header[i] + 1);
    } else {
      s->write(header[i]);
    }
  }
  for (int i = 0; i < length; ++i) {
    if (i == data_error_index) {
      s->write(contents[i] + 1);
    } else {
      s->write(contents[i]);
    }
  }
}

void WritePacket(const Ack &ack, const unsigned char index, SerialInterface *s) {
  WritePacketError(ack, index, 200, 200, s);
}

void WritePacketDataError(const Ack &ack, const unsigned char index, const unsigned char error_index, SerialInterface *s) {
  WritePacketError(ack, index, 200, error_index, s);
}

void WritePacketHeaderError(const Ack &ack, const unsigned char index, const unsigned char error_index, SerialInterface *s) {
  WritePacketError(ack, index, error_index, 200, s);
}

TEST(ReaderTest, ReadOnePacket) {
  FakeArduino s0, s1;
  ASSERT_TRUE(s0.UseFiles("/tmp/read_one_packet_a", "/tmp/read_one_packet_b"));
  ASSERT_TRUE(s1.UseFiles("/tmp/read_one_packet_b", "/tmp/read_one_packet_a"));
  Reader reader(&s1);
  EXPECT_FALSE(reader.Read());
  EXPECT_FALSE(reader.Read());
  WritePacket(Ack(0x72), 1, &s0);
  while (reader.Read());
  Packet *popped = reader.PopPacket();
  ASSERT_NE(popped, nullptr);
  Ack incoming_ack = reader.PopIncomingAck();
  EXPECT_EQ(incoming_ack.index(), 1);
  EXPECT_EQ(incoming_ack.error(), false);
  EXPECT_EQ(reader.PopOutgoingAck().index(), 0x72);
  unsigned char length;
  EXPECT_EQ(popped->data(&length)[0], 2);
  EXPECT_EQ(length, 1);
}

TEST(ReaderTest, SendErrorForValidPacketBadIndex) {
  FakeArduino s0, s1;
  ASSERT_TRUE(s0.UseFiles("/tmp/bad_index_a", "/tmp/bad_index_b"));
  ASSERT_TRUE(s1.UseFiles("/tmp/bad_index_b", "/tmp/bad_index_a"));
  Reader reader(&s1);
  EXPECT_FALSE(reader.Read());
  WritePacket(Ack(0x72), 7, &s0);
  while (reader.Read());
  Packet *popped = reader.PopPacket();
  EXPECT_EQ(popped, nullptr);
  Ack incoming_ack = reader.PopIncomingAck();
  EXPECT_EQ(incoming_ack.index(), 0);
  EXPECT_EQ(reader.PopOutgoingAck().index(), 0x72);
}

TEST(ReaderTest, SendErrorForInvalidDataPacket) {
  FakeArduino s0, s1;
  ASSERT_TRUE(s0.UseFiles("/tmp/read_one_invalid_data_packet_a", "/tmp/read_one_invalid_data_packet_b"));
  ASSERT_TRUE(s1.UseFiles("/tmp/read_one_invalid_data_packet_b", "/tmp/read_one_invalid_data_packet_a"));
  Reader reader(&s1);
  EXPECT_FALSE(reader.Read());
  EXPECT_FALSE(reader.Read());
  for (int error_index = 0; error_index < 3; ++error_index) {
    WritePacketDataError(Ack(0x72), 1, error_index, &s0);
    while (reader.Read());
    Packet *popped = reader.PopPacket();
    EXPECT_EQ(popped, nullptr);
    Ack incoming_ack = reader.PopIncomingAck();
    EXPECT_EQ(incoming_ack.index(), 1);
    EXPECT_EQ(incoming_ack.error(), true);
    EXPECT_EQ(reader.PopOutgoingAck().index(), 0x72);
  }
}

TEST(ReaderTest, IgnoreInvalidHeader) {
  FakeArduino s0, s1;
  ASSERT_TRUE(s0.UseFiles("/tmp/read_one_invalid_header_packet_a", "/tmp/read_one_invalid_header_packet_b"));
  ASSERT_TRUE(s1.UseFiles("/tmp/read_one_invalid_header_packet_b", "/tmp/read_one_invalid_header_packet_a"));
  Reader reader(&s1);
  EXPECT_FALSE(reader.Read());
  EXPECT_FALSE(reader.Read());
  for (int error_index = 0; error_index < 7; ++error_index) {
    WritePacketHeaderError(Ack(0x72), 1, error_index, &s0);
    while (reader.Read());
    Packet *popped = reader.PopPacket();
    EXPECT_EQ(popped, nullptr);
    Ack incoming_ack = reader.PopIncomingAck();
    EXPECT_EQ(incoming_ack.index(), 0);
    reader.PopOutgoingAck();
  }
}

TEST(ReaderTest, HandleDuplicate1060) {
  FakeArduino s0, s1;
  ASSERT_TRUE(s0.UseFiles("/tmp/read_one_packet_a", "/tmp/read_one_packet_b"));
  ASSERT_TRUE(s1.UseFiles("/tmp/read_one_packet_b", "/tmp/read_one_packet_a"));
  Reader reader(&s1);
  EXPECT_FALSE(reader.Read());
  EXPECT_FALSE(reader.Read());
  // Write some initial junk, so we make sure it doesn't foul up the subsequent message.
  s0.write(10);
  s0.write(60);
  s0.write(23);
  WritePacket(Ack(0x72), 1, &s0);
  while (reader.Read());
  Packet *popped = reader.PopPacket();
  ASSERT_NE(popped, nullptr);
  Ack incoming_ack = reader.PopIncomingAck();
  EXPECT_EQ(incoming_ack.index(), 1);
  EXPECT_EQ(incoming_ack.error(), false);
  EXPECT_EQ(reader.PopOutgoingAck().index(), 0x72);
  unsigned char length;
  EXPECT_EQ(popped->data(&length)[0], 2);
  EXPECT_EQ(length, 1);
}

TEST(ReaderTest, ReadMany) {
}

}  // namespace
}  // namespace tensixty
