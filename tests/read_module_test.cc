#include <gtest/gtest.h>
#include "cc/read_module.h"
#include "tests/arduino_simulator.h"
#include "cc/module_dispatcher.h"

#include <vector>

namespace markbot {
namespace {

bool ParseResponse(const Message &response, IOReadProto *read_proto) {
  EXPECT_GT(response.length(), 1);
  EXPECT_EQ(response.type(), READ_REPORT);
  PARSE_OR_RETURN(IOReadProto, report_internal, response.data(), response.length());
  *read_proto = report_internal;
  return true;
}

TEST(ReadModule, Initialize) {
  tensixty::FakeArduino arduino;
  ReadModule reader(&arduino);
  Message message = reader.Tick();
  EXPECT_EQ(message.length(), 0);
  EXPECT_EQ(message.data(), nullptr);
}

TEST(ReadModule, ReadInput) {
  tensixty::FakeArduino arduino;
  ReadModule reader(&arduino);
  IOReadRequestProto req;
  req.input_pin_bitmap = 0x1085;
  req.pullup_pin_bitmap = 0x00;
  uint8_t buffer[50];
  int bytes_written;
  SERIALIZE(IOReadRequestProto, req, buffer, bytes_written);
  ASSERT_GT(bytes_written, 0);
  Message message(READ_REQUEST, bytes_written, buffer);
  for (int i = 1; i < 64; ++i) {
    arduino.setPinModeOutput(i);
  }
  EXPECT_TRUE(reader.AcceptMessage(message));
  std::vector<int> input_pins = {1, 3, 8, 13};
  for (int i = 1; i < 64; ++i) {
    if (std::find(input_pins.begin(), input_pins.end(), i) == input_pins.end()) {
      EXPECT_TRUE(arduino.testIsPinOutput(i)) << i;
    } else {
      EXPECT_FALSE(arduino.testIsPinOutput(i)) << i;
    }
    EXPECT_FALSE(arduino.testIsPinPullup(i)) << i;
  }
  arduino.testSetPinInput(3, true);
  arduino.testSetPinInput(9, false);

  Message response = reader.Tick();
  IOReadProto read_result;
  ASSERT_TRUE(ParseResponse(response, &read_result));
  EXPECT_EQ(read_result.pin_states_bitmap, 0x04);
  arduino.testSetPinInput(13, true);
  Message response2 = reader.Tick();
  IOReadProto read_result2;
  ASSERT_TRUE(ParseResponse(response2, &read_result2));
  EXPECT_EQ(read_result2.pin_states_bitmap, 0x1004);

  // No change -> no update
  Message response3 = reader.Tick();
  EXPECT_EQ(response3.type(), 0);
}

TEST(ReadModule, ReadPullup) {
  tensixty::FakeArduino arduino;
  ReadModule reader(&arduino);
  IOReadRequestProto req;
  req.pullup_pin_bitmap = 0x1085;
  req.input_pin_bitmap = 0x00;
  uint8_t buffer[50];
  int bytes_written;
  SERIALIZE(IOReadRequestProto, req, buffer, bytes_written);
  ASSERT_GT(bytes_written, 0);
  Message message(READ_REQUEST, bytes_written, buffer);
  for (int i = 1; i < 64; ++i) {
    arduino.setPinModeOutput(i);
  }
  EXPECT_TRUE(reader.AcceptMessage(message));
  std::vector<int> pullup_pins = {1, 3, 8, 13};
  for (int i = 1; i < 64; ++i) {
    if (std::find(pullup_pins.begin(), pullup_pins.end(), i) == pullup_pins.end()) {
      EXPECT_TRUE(arduino.testIsPinOutput(i)) << i;
    } else {
      EXPECT_FALSE(arduino.testIsPinOutput(i)) << i;
      EXPECT_TRUE(arduino.testIsPinPullup(i)) << i;
    }
  }

  // Add more pins
  req.input_pin_bitmap = 0x700000;
  SERIALIZE(IOReadRequestProto, req, buffer, bytes_written);
  ASSERT_GT(bytes_written, 0);
  Message message2(READ_REQUEST, bytes_written, buffer);
  EXPECT_TRUE(reader.AcceptMessage(message2));
  std::vector<int> input_pins = {21, 22, 23};
  for (int i = 1; i < 64; ++i) {
    if (std::find(pullup_pins.begin(), pullup_pins.end(), i) != pullup_pins.end()) {
      EXPECT_TRUE(arduino.testIsPinPullup(i)) << i;
      EXPECT_FALSE(arduino.testIsPinOutput(i)) << i;
    } else if (std::find(input_pins.begin(), input_pins.end(), i) != input_pins.end()) {
      EXPECT_FALSE(arduino.testIsPinPullup(i)) << i;
      EXPECT_FALSE(arduino.testIsPinOutput(i)) << i;
    } else {
      EXPECT_FALSE(arduino.testIsPinPullup(i)) << i;
      EXPECT_TRUE(arduino.testIsPinOutput(i)) << i;
    }
  }
}

}  // namespace
}  // namespace markbot
