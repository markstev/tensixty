// Using https://github.com/google/googletest

#include <gtest/gtest.h>
#include "commlink.h"

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
  unsigned char length;
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
    printf("Trial %d\n", i);
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
    printf("Trial %d\n", i);
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

}  // namespace
}  // namespace tensixty
