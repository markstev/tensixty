// Using https://github.com/google/googletest

#include <gtest/gtest.h>
#include "cc/packet.h"

namespace tensixty {
namespace {

TEST(AckTest, SerializeDeserialize) {
  Ack a;
  EXPECT_EQ(a.index(), 0);
  a.Parse(0x81);
  EXPECT_EQ(a.index(), 1);
  EXPECT_EQ(a.error(), true);
  EXPECT_EQ(a.ok(), false);
  a.Parse(0x71);
  EXPECT_EQ(a.index(), 0x71);
  EXPECT_EQ(a.error(), false);
  EXPECT_EQ(a.ok(), true);
  a.Parse(0xF1);
  EXPECT_EQ(a.index(), 0x71);
  EXPECT_EQ(a.error(), true);
  EXPECT_EQ(a.ok(), false);

  Ack b;
  EXPECT_EQ(b.index(), 0);
  b = a;
  EXPECT_EQ(b.index(), 0x71);
  EXPECT_EQ(b.error(), true);
  EXPECT_EQ(b.ok(), false);
  a.Parse(0x10);
  EXPECT_EQ(b.index(), 0x71);
  EXPECT_EQ(b.error(), true);
  EXPECT_EQ(b.ok(), false);
  EXPECT_EQ(a.index(), 0x10);
  EXPECT_EQ(a.error(), false);
  EXPECT_EQ(a.ok(), true);
}

TEST(PacketTest, ParseOkay) {
  unsigned char header[7], data[15];
  unsigned int data_bytes;
  const unsigned char MESSAGE_SIZE = 4;
  const unsigned char message_to_send[MESSAGE_SIZE] = {0x23, 0xFF, 0x01, 0x88};
  {
    Packet original;
    original.IncludeAck(Ack(0xF1));
    original.IncludeData(22, message_to_send, MESSAGE_SIZE);
    EXPECT_EQ(original.ack().error(), true);
    EXPECT_EQ(original.ack().index(), 0x71);
    EXPECT_EQ(original.index_sending(), 22);
    unsigned char length;
    const unsigned char *contents = original.data(&length);
    EXPECT_EQ(length, MESSAGE_SIZE);
    for (int i = 0; i < length; ++i) {
      EXPECT_EQ(contents[i], message_to_send[i]);
    }

    original.Serialize(header, data, &data_bytes);
    EXPECT_EQ(data_bytes, MESSAGE_SIZE + 2);
  }
  {
    Packet parsed;
    for (int i = 0; i < 7; ++i) {
      EXPECT_EQ(INCOMPLETE, parsed.ParseChar(header[i]));
      EXPECT_FALSE(parsed.error());
      EXPECT_FALSE(parsed.parsed());
    }

    EXPECT_EQ(parsed.ack().error(), true);
    EXPECT_EQ(parsed.ack().index(), 0x71);
    EXPECT_EQ(parsed.index_sending(), 22);

    for (int i = 0; i < MESSAGE_SIZE + 1; ++i) {
      EXPECT_EQ(INCOMPLETE, parsed.ParseChar(data[i]));
      EXPECT_FALSE(parsed.error());
      EXPECT_FALSE(parsed.parsed());
    }
    EXPECT_EQ(PARSED, parsed.ParseChar(data[MESSAGE_SIZE + 1]));
    EXPECT_FALSE(parsed.error());
    EXPECT_TRUE(parsed.parsed());
    EXPECT_EQ(parsed.ack().error(), true);
    EXPECT_EQ(parsed.ack().index(), 0x71);
    EXPECT_EQ(parsed.index_sending(), 22);
    unsigned char length;
    const unsigned char *contents = parsed.data(&length);
    EXPECT_EQ(length, MESSAGE_SIZE);
    for (int i = 0; i < length; ++i) {
      EXPECT_EQ(contents[i], message_to_send[i]);
    }
  }
}

TEST(PacketTest, ParseHeaderNotOkay) {
  for (int i = 0; i < 7; ++i) {
    unsigned char header[7], data[15];
    unsigned int data_bytes;
    const unsigned char MESSAGE_SIZE = 4;
    const unsigned char message_to_send[MESSAGE_SIZE] = {0x23, 0xFF, 0x01, 0x88};
    {
      Packet original;
      original.IncludeAck(Ack(0xF1));
      original.IncludeData(22, message_to_send, MESSAGE_SIZE);
      unsigned char length;
      original.data(&length);
      original.Serialize(header, data, &data_bytes);
    }
    header[i] += 1;
    {
      Packet parsed;
      if (i == 0) {
        EXPECT_EQ(HEADER_ERROR, parsed.ParseChar(header[0]));
        continue;
      } else if (i == 1) {
        EXPECT_EQ(INCOMPLETE, parsed.ParseChar(header[0]));
        EXPECT_EQ(HEADER_ERROR, parsed.ParseChar(header[1]));
        continue;
      }
      for (int j = 0; j < 5; ++j) {
        EXPECT_EQ(INCOMPLETE, parsed.ParseChar(header[j]));
      }
      if (i != 6) {
        EXPECT_EQ(HEADER_ERROR, parsed.ParseChar(header[5]));
      } else {
        EXPECT_EQ(INCOMPLETE, parsed.ParseChar(header[5]));
        EXPECT_EQ(HEADER_ERROR, parsed.ParseChar(header[6]));
      }
    }
  }
}

TEST(PacketTest, ParseDataNotOkay) {
  static const unsigned int MESSAGE_SIZE = 255;
  unsigned char message_to_send[MESSAGE_SIZE];
  for (int i = 0; i < MESSAGE_SIZE; ++i) {
    message_to_send[i] = i;
  }

  for (int x = 0; x < MESSAGE_SIZE; ++x) {
    unsigned char header[7], data[MESSAGE_SIZE + 2];
    unsigned int data_bytes;
    {
      Packet original;
      original.IncludeAck(Ack(0xF1));
      original.IncludeData(22, message_to_send, MESSAGE_SIZE);
      original.Serialize(header, data, &data_bytes);
    }
    data[x] += 1;
    Packet parsed;
    for (int i = 0; i < 7; ++i) {
      EXPECT_EQ(INCOMPLETE, parsed.ParseChar(header[i]));
      EXPECT_FALSE(parsed.error());
      EXPECT_FALSE(parsed.parsed());
    }

    EXPECT_EQ(parsed.ack().error(), true);
    EXPECT_EQ(parsed.ack().index(), 0x71);
    EXPECT_EQ(parsed.index_sending(), 22);

    for (int i = 0; i < MESSAGE_SIZE; ++i) {
      EXPECT_EQ(INCOMPLETE, parsed.ParseChar(data[i]));
    }
    bool found_error = false;
    for (int i = 0; i < 2; ++i) {
      if (DATA_ERROR == parsed.ParseChar(data[MESSAGE_SIZE + i])) {
        found_error = true;
        break;
      }
    }
    EXPECT_TRUE(found_error);
    EXPECT_TRUE(parsed.error());
  }
}

}  // namespace
}  // namespace tensixty
