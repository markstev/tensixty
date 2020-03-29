#include <gtest/gtest.h>
#include "commlink.h"
#include "arduino_simulator.h"
#include "serial_interface.h"

namespace tensixty {
namespace {

TEST(PariTest, SendBothWays) {
  FakeArduino s0, s1;
  ASSERT_TRUE(s0.UseFiles("/tmp/send_bidir_a", "/tmp/send_bidir_b"));
  ASSERT_TRUE(s1.UseFiles("/tmp/send_bidir_b", "/tmp/send_bidir_a"));
  RxTxPair p0(&s0);
  RxTxPair p1(&s1);

  static const int NUM_TO_SEND = 3;
  const unsigned char p0_to_send[NUM_TO_SEND][5] = {
    {1, 2, 3, 4, 5},
    {6, 7, 8, 9, 0},
    {1, 2, 1, 1, 2},
  };
  const unsigned char p1_to_send[NUM_TO_SEND][2] = {
    {8, 2},
    {99, 7},
    {1, 200},
  };
  int p0_send_index = 0;
  int p1_send_index = 0;
  int p0_receive_index = 0;
  int p1_receive_index = 0;
  while (p0_send_index < NUM_TO_SEND ||
         p1_send_index < NUM_TO_SEND ||
         p0_receive_index < NUM_TO_SEND ||
         p1_receive_index < NUM_TO_SEND) {
    p0.Tick();
    p1.Tick();
    if (p0_send_index < NUM_TO_SEND) {
      if (p0.Transmit(p0_to_send[p0_send_index], 5)) {
        p0_send_index++;
      }
    }
    if (p1_send_index < NUM_TO_SEND) {
      if (p1.Transmit(p1_to_send[p1_send_index], 3)) {
        p1_send_index++;
      }
    }
    unsigned char length;
    const unsigned char *data = p0.Receive(&length);
    if (length > 0) {
      EXPECT_EQ(length, 3);
      EXPECT_LT(p0_receive_index, NUM_TO_SEND);
      for (int i = 0; i < 2; ++i) {
        EXPECT_EQ(p1_to_send[p0_receive_index][i], data[i]);
      }
      ++p0_receive_index;
    }
    data = p1.Receive(&length);
    if (length > 0) {
      EXPECT_EQ(length, 5);
      EXPECT_LT(p1_receive_index, NUM_TO_SEND);
      for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(p0_to_send[p1_receive_index][i], data[i]);
      }
      ++p1_receive_index;
    }
  }
  EXPECT_EQ(NUM_TO_SEND, p0_send_index);
  EXPECT_EQ(NUM_TO_SEND, p1_send_index);
  EXPECT_EQ(NUM_TO_SEND, p0_receive_index);
  EXPECT_EQ(NUM_TO_SEND, p1_receive_index);
}

}  // namespace
}  // namespace tensixty
