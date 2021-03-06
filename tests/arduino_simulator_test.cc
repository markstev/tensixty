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

TEST(HardwareAbstractionTest, pinModes) {
  FakeArduino arduino;
  arduino.setPinModeOutput(17);
  EXPECT_EQ(arduino.testGetPinMode(17), OUTPUT);
  arduino.setPinModeInput(17);
  EXPECT_EQ(arduino.testGetPinMode(17), INPUT);
  arduino.testSetPinInput(17, true);
  EXPECT_EQ(arduino.digitalRead(17), true);
  arduino.testSetPinInput(17, false);
  EXPECT_EQ(arduino.digitalRead(17), false);
  arduino.setPinModeOutput(17);
  arduino.digitalWrite(17, true);
  EXPECT_EQ(arduino.testGetPinOutput(17), true);
  arduino.digitalWrite(17, false);
  EXPECT_EQ(arduino.testGetPinOutput(17), false);
}

TEST(HardwareAbstractionTest, SerialIO) {
  FakeArduino s0, s1;
  ASSERT_TRUE(s0.UseFiles("/tmp/test_serial_io_a", "/tmp/test_serial_io_b"));
  ASSERT_TRUE(s1.UseFiles("/tmp/test_serial_io_b", "/tmp/test_serial_io_a"));
  EXPECT_FALSE(s0.available());
  EXPECT_FALSE(s1.available());
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
