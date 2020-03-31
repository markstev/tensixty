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
  // TODO: Handle duplicate 1060's
  //s0.write(10);
  //s0.write(60);
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
  FakeArduino s0, s1;
  ASSERT_TRUE(s0.UseFiles("/tmp/read_many_a", "/tmp/read_many_b"));
  ASSERT_TRUE(s1.UseFiles("/tmp/read_many_b", "/tmp/read_many_a"));
  Reader reader(&s1);
  EXPECT_FALSE(reader.Read());
  EXPECT_FALSE(reader.Read());
  for (int i = 0; i < 300; ++i) {
    WritePacket(Ack(0x72), ToIndex(i), &s0);
    while (reader.Read());
    Packet *popped = reader.PopPacket();
    ASSERT_NE(popped, nullptr);
    Ack incoming_ack = reader.PopIncomingAck();
    EXPECT_EQ(incoming_ack.index(), ToIndex(i));
    EXPECT_EQ(incoming_ack.error(), false);
    EXPECT_EQ(reader.PopOutgoingAck().index(), 0x72);
    unsigned char length;
    EXPECT_EQ(popped->data(&length)[0], ToIndex(i) + 1);
    EXPECT_EQ(length, 1);
  }
}

TEST(OutgoingPacketBufferTest, AllocateUntilFull) {
  OutgoingPacketBuffer b;
  for (int i = 0; i < 10; ++i) {
    Packet *p = b.AllocatePacket();
    if (i < 4) {
      EXPECT_NE(p, nullptr);
    } else {
      EXPECT_EQ(p, nullptr);
    }
  }
}

TEST(OutgoingPacketBufferTest, NothingToResend) {
  OutgoingPacketBuffer b;
  for (int i = 0; i < 10; ++i) {
    Packet *p = b.AllocatePacket();
    if (i < 4) {
      EXPECT_NE(p, nullptr);
    } else {
      EXPECT_EQ(p, nullptr);
    }
  }
}

TEST(OutgoingPacketBufferTest, NextPacket) {
  OutgoingPacketBuffer b;
  for (int i = 0; i < 10; ++i) {
    Packet *p = b.AllocatePacket();
    if (i < 4) {
      ASSERT_NE(p, nullptr);
      FillPacket(Ack(0x72), i + 1, p);
    } else {
      EXPECT_EQ(p, nullptr);
    }
    ASSERT_NE(b.NextPacket(), nullptr);
    EXPECT_EQ(1, b.NextPacket()->index_sending());
  }
}

TEST(OutgoingPacketBufferTest, RemovePacket) {
  OutgoingPacketBuffer b;
  for (int i = 0; i < 4; ++i) {
    Packet *p = b.AllocatePacket();
    FillPacket(Ack(0x72), i + 1, p);
  }
  for (int i = 0; i < 4; ++i) {
    ASSERT_NE(b.NextPacket(), nullptr);
    EXPECT_EQ(i + 1, b.NextPacket()->index_sending());
    b.RemovePacket(i + 1);
  }
  EXPECT_EQ(b.NextPacket(), nullptr);
}

TEST(OutgoingPacketBufferTest, PeekPacket) {
  OutgoingPacketBuffer b;
  for (int i = 0; i < 4; ++i) {
    Packet *p = b.AllocatePacket();
    FillPacket(Ack(0x72), i + 1, p);
  }
  for (int i = 0; i < 10; ++i) {
    Packet *p = b.PeekPacket(i + 1);
    if (i < 4) {
      ASSERT_NE(p, nullptr);
      EXPECT_EQ(p->index_sending(), i + 1);
    } else {
      EXPECT_EQ(p, nullptr);
    }
  }
  b.RemovePacket(2);
  b.RemovePacket(4);
  EXPECT_NE(b.PeekPacket(1), nullptr);
  EXPECT_NE(b.PeekPacket(3), nullptr);
  EXPECT_EQ(b.PeekPacket(2), nullptr);
  EXPECT_EQ(b.PeekPacket(4), nullptr);
  ASSERT_NE(b.NextPacket(), nullptr);
  EXPECT_EQ(b.NextPacket()->index_sending(), 1);
}

TEST(OutgoingPacketBufferTest, RemoveOutOfOrderMakesResends) {
  OutgoingPacketBuffer b;
  for (int i = 0; i < 4; ++i) {
    Packet *p = b.AllocatePacket();
    FillPacket(Ack(0x72), i + 1, p);
  }
  b.RemovePacket(2);
  b.RemovePacket(4);
  Packet *p = b.PeekResendPacket();
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(p->index_sending(), 1);
  p = b.PeekResendPacket();
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(p->index_sending(), 3);
}

TEST(OutgoingPacketBufferTest, AddRemovePacketLoop) {
  OutgoingPacketBuffer b;
  for (int i = 0; i < 300; ++i) {
    printf("Trial %d\n", i);
    Packet *p = b.AllocatePacket();
    FillPacket(Ack(0x72), ToIndex(i), p);
    p = b.NextPacket();
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->index_sending(), ToIndex(i));
    b.RemovePacket(ToIndex(i));
  }
}

TEST(OutgoingPacketBufferTest, AddRemovePacketLoopWithBuffer) {
  for (int gap = 1; gap < 4; ++gap) {
    OutgoingPacketBuffer b;
    for (int i = 0; i < 300; ++i) {
      printf("Trial %d\n", i);
      Packet *p = b.AllocatePacket();
      FillPacket(Ack(0x72), ToIndex(i), p);

      if (i >= gap) {
        p = b.NextPacket();
        ASSERT_NE(p, nullptr);
        EXPECT_EQ(p->index_sending(), ToIndex(i - gap));
        b.RemovePacket(ToIndex(i - gap));
      }
    }
  }
}

class FakeAcker : public AckProvider {
 public:
  Ack PopIncomingAck() override {
    Ack incoming = incoming_ack_;
    incoming_ack_.Parse(0x00);
    return incoming;
  }
  Ack PopOutgoingAck() override {
    Ack outgoing = outgoing_ack_;
    outgoing_ack_.Parse(0x00);
    return outgoing;
  }

  void WithIncoming(const Ack &ack) {
    incoming_ack_ = ack;
  }

  void WithOutgoing(const Ack &ack) {
    outgoing_ack_ = ack;
  }
 private:
  Ack incoming_ack_;
  Ack outgoing_ack_;
};

TEST(WriterTest, AddToOutgoingQueue) {
  FakeAcker reader;
  FakeArduino s0, s1;
  ASSERT_TRUE(s0.UseFiles("/tmp/writer_add_outgoing_a", "/tmp/writer_add_outgoing_b"));
  ASSERT_TRUE(s1.UseFiles("/tmp/writer_add_outgoing_b", "/tmp/writer_add_outgoing_a"));
  Writer writer(*GetRealClock(), &s1, &reader);
  for (int i = 0; i < 10; ++i) {
    const unsigned char data[3] = {4, 9, 17};
    const bool added = writer.AddToOutgoingQueue(data, 3);
    EXPECT_EQ(added, i < 4);
  }
}

TEST(WriterTest, WriteOne) {
  FakeAcker reader;
  FakeArduino s0, s1;
  ASSERT_TRUE(s0.UseFiles("/tmp/writer_write_one_a", "/tmp/writer_write_one_b"));
  ASSERT_TRUE(s1.UseFiles("/tmp/writer_write_one_b", "/tmp/writer_write_one_a"));
  Writer writer(*GetRealClock(), &s1, &reader);
  const unsigned char data[3] = {4, 9, 17};
  EXPECT_TRUE(writer.AddToOutgoingQueue(data, 3));
  EXPECT_TRUE(writer.Write());

  Reader real_reader(&s0);
  while (real_reader.Read());
  Packet *p = real_reader.PopPacket();
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(p->index_sending(), 1);
  // Nothing to ack yet
  EXPECT_EQ(p->ack().index(), 0);
  unsigned char length;
  EXPECT_EQ(p->data(&length)[0], 4);
  EXPECT_EQ(length, 3);
  EXPECT_EQ(p->data(&length)[1], 9);
  EXPECT_EQ(p->data(&length)[2], 17);

  EXPECT_EQ(real_reader.PopOutgoingAck().index(), 0);
  const Ack incoming_ack = real_reader.PopIncomingAck();
  EXPECT_EQ(incoming_ack.index(), 1);
  EXPECT_TRUE(incoming_ack.ok());
  while (real_reader.Read());
  p = real_reader.PopPacket();
  EXPECT_EQ(p, nullptr);
}

TEST(WriterTest, WriteMany) {
  FakeAcker acker;
  FakeArduino s0, s1;
  ASSERT_TRUE(s0.UseFiles("/tmp/writer_write_many_a", "/tmp/writer_write_many_b"));
  ASSERT_TRUE(s1.UseFiles("/tmp/writer_write_many_b", "/tmp/writer_write_many_a"));
  const unsigned char data[3] = {4, 9, 17};
  for (int ack_gap = 0; ack_gap < 3; ++ack_gap) {
    Writer writer(*GetRealClock(), &s1, &acker);
    Reader real_reader(&s0);
    for (int i = 0; i < 300; ++i) {
      printf("Trial %d\n", i);
      EXPECT_TRUE(writer.AddToOutgoingQueue(data, 3));
      EXPECT_TRUE(writer.Write());

      while (real_reader.Read());
      Packet *p = real_reader.PopPacket();
      ASSERT_NE(p, nullptr);
      EXPECT_EQ(p->index_sending(), ToIndex(i));
      // Nothing to ack yet
      EXPECT_EQ(p->ack().index(), 0);
      unsigned char length;
      EXPECT_EQ(p->data(&length)[0], 4);
      EXPECT_EQ(length, 3);
      EXPECT_EQ(p->data(&length)[1], 9);
      EXPECT_EQ(p->data(&length)[2], 17);

      EXPECT_EQ(real_reader.PopOutgoingAck().index(), 0);
      const Ack incoming_ack = real_reader.PopIncomingAck();
      EXPECT_EQ(incoming_ack.index(), ToIndex(i));
      EXPECT_TRUE(incoming_ack.ok());

      while (real_reader.Read());
      p = real_reader.PopPacket();
      EXPECT_EQ(p, nullptr);

      if (i >= ack_gap) {
        acker.WithOutgoing(Ack(ToIndex(i - ack_gap)));
      }
    }
  }
}

TEST(WriterTest, Resend) {
  FakeAcker acker;
  FakeArduino s0, s1;
  ASSERT_TRUE(s0.UseFiles("/tmp/writer_resend_a", "/tmp/writer_resend_b"));
  ASSERT_TRUE(s1.UseFiles("/tmp/writer_resend_b", "/tmp/writer_resend_a"));
  Writer writer(*GetRealClock(), &s1, &acker);
  const unsigned char data[3] = {4, 9, 17};
  EXPECT_TRUE(writer.AddToOutgoingQueue(data, 3));
  EXPECT_TRUE(writer.Write());

  Reader real_reader(&s0);
  while (real_reader.Read());
  Packet *p = real_reader.PopPacket();
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(p->index_sending(), 1);
  // Nothing to ack yet
  EXPECT_EQ(p->ack().index(), 0);
  unsigned char length;
  EXPECT_EQ(p->data(&length)[0], 4);
  EXPECT_EQ(length, 3);
  EXPECT_EQ(p->data(&length)[1], 9);
  EXPECT_EQ(p->data(&length)[2], 17);

  EXPECT_EQ(real_reader.PopOutgoingAck().index(), 0);
  const Ack incoming_ack = real_reader.PopIncomingAck();
  EXPECT_EQ(incoming_ack.index(), 1);
  EXPECT_TRUE(incoming_ack.ok());
  while (real_reader.Read());
  p = real_reader.PopPacket();
  EXPECT_EQ(p, nullptr);

  EXPECT_FALSE(writer.Write());

  acker.WithOutgoing(Ack(ToIndex(0) | 0x80));
  EXPECT_TRUE(writer.Write());
  Reader real_reader2(&s0);
  while (real_reader2.Read());
  p = real_reader2.PopPacket();
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(p->ack().index(), 0);
  EXPECT_EQ(p->data(&length)[0], 4);
  EXPECT_EQ(length, 3);
}

TEST(WriterTest, OutOfOrderTriggersResends) {
  FakeAcker acker;
  FakeArduino s0, s1;
  ASSERT_TRUE(s0.UseFiles("/tmp/writer_out_of_order_a", "/tmp/writer_out_of_order_b"));
  ASSERT_TRUE(s1.UseFiles("/tmp/writer_out_of_order_b", "/tmp/writer_out_of_order_a"));
  const unsigned char data[3] = {4, 9, 17};
  for (int ack_gap = 0; ack_gap < 3; ++ack_gap) {
    Writer writer(*GetRealClock(), &s1, &acker);
    Reader real_reader(&s0);
    const int num_trials = 300;
    for (int i = 0; i < num_trials; ++i) {
      printf("Trial %d\n", i);
      EXPECT_TRUE(writer.AddToOutgoingQueue(data, 3));
      EXPECT_TRUE(writer.Write());

      if (num_trials - 1 - i <= ack_gap) {
        s0.Mute();
      } else {
        while (real_reader.Read());
        Packet *p = real_reader.PopPacket();
        ASSERT_NE(p, nullptr);

        const Ack incoming_ack = real_reader.PopIncomingAck();
        EXPECT_EQ(incoming_ack.index(), ToIndex(i));
        EXPECT_EQ(real_reader.PopOutgoingAck().index(), 0);
      }
      while (real_reader.Read());
      Packet *p = real_reader.PopPacket();
      EXPECT_EQ(p, nullptr);

      if (i >= ack_gap) {
        acker.WithOutgoing(Ack(ToIndex(i - ack_gap)));
        printf("Ack trial: %d\n", i - ack_gap);
      }
    }
    s0.UnMute();
    // Ack the last index implying that messages from num_trials - ack_gap - 1
    // to num_trials - 2 need to be resent.
    acker.WithOutgoing(Ack(ToIndex(num_trials - 1)));
    for (int j = num_trials - ack_gap - 1; j < num_trials - 1; ++j) {
      printf("Resend for trial: %d and gap %d\n", j, ack_gap);
      writer.Write();
      while (real_reader.Read());
      Packet *p = real_reader.PopPacket();
      ASSERT_NE(p, nullptr);
      EXPECT_EQ(p->index_sending(), ToIndex(j));
      const Ack incoming_ack = real_reader.PopIncomingAck();
      EXPECT_EQ(incoming_ack.index(), ToIndex(j));
      EXPECT_EQ(real_reader.PopOutgoingAck().index(), 0);
      acker.WithOutgoing(Ack(ToIndex(j)));
    }
    while (real_reader.Read());
    Packet *p = real_reader.PopPacket();
    EXPECT_EQ(p, nullptr);
    EXPECT_EQ(real_reader.PopOutgoingAck().index(), 0);
    EXPECT_EQ(real_reader.PopIncomingAck().index(), 0);
  }
}

TEST(WriterTest, SendAckOnly) {
  FakeAcker acker;
  FakeArduino s0, s1;
  ASSERT_TRUE(s0.UseFiles("/tmp/writer_ack_only_a", "/tmp/writer_ack_only_b"));
  ASSERT_TRUE(s1.UseFiles("/tmp/writer_ack_only_b", "/tmp/writer_ack_only_a"));
  acker.WithIncoming(Ack(0x71));
  Writer writer(*GetRealClock(), &s1, &acker);
  Reader reader(&s0);
  writer.Write();
  while (reader.Read());
  Packet *p = reader.PopPacket();
  EXPECT_EQ(p, nullptr);
  EXPECT_EQ(reader.PopIncomingAck().index(), 0);
  EXPECT_EQ(reader.PopOutgoingAck().index(), 0x71);
}

}  // namespace
}  // namespace tensixty
