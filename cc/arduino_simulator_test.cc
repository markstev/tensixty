// Using https://github.com/google/googletest

#include <gtest/gtest.h>
#include "arduino_simulator.h"

namespace tensixty {
namespace {

TEST(HardwareAbstractionTest, digitalWriteSomePins) {
  // Just makes sure it doesn't crash.
  FakeArduino arduino;
  arduino.digitalWrite(1, true);
  arduino.digitalWrite(1, false);
  EXPECT_EQ(false, arduino.digitalRead(2));
  EXPECT_EQ(false, arduino.digitalRead(1));
  arduino.digitalWrite(1, true);
  EXPECT_EQ(true, arduino.digitalRead(1));
  arduino.digitalWrite(10, true);
  EXPECT_EQ(true, arduino.digitalRead(10));
}

TEST(HardwareAbstractionTest, digitalWriteSomePins2) {
  FakeArduino arduino;
  arduino.digitalWrite(10, true);
  EXPECT_EQ(true, arduino.digitalRead(10));
  arduino.digitalWrite(10, false);
  EXPECT_EQ(false, arduino.digitalRead(10));
}

TEST(HardwareAbstractionTest, SerialIO) {
  FakeArduino s0, s1;
  ASSERT_TRUE(s0.UseFiles("/tmp/test_serial_io_a", "/tmp/test_serial_io_b"));
  ASSERT_TRUE(s1.UseFiles("/tmp/test_serial_io_b", "/tmp/test_serial_io_a"));
  EXPECT_FALSE(s0.available());
  EXPECT_FALSE(s1.available());
  EXPECT_EQ(-1, s0.read());
  EXPECT_EQ(-1, s1.read());
  s0.write(0x42);
  EXPECT_EQ(s1.read(), 0x42);
  EXPECT_FALSE(s1.available());
  s0.write(0x44);
  EXPECT_TRUE(s1.available());
  EXPECT_EQ(s1.read(), 0x44);
  s1.write(0x01);
  s1.write(0x02);
  s1.write(0x03);
  EXPECT_EQ(s0.read(), 0x01);
  EXPECT_EQ(s0.read(), 0x02);
  EXPECT_EQ(s0.read(), 0x03);
  EXPECT_FALSE(s0.available());
  s1.write(0x04);
  EXPECT_TRUE(s0.available());
  EXPECT_EQ(s0.read(), 0x04);
}

}  // namespace
}  // namespace tensixty
