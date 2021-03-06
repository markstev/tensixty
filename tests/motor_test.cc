#include <gtest/gtest.h>
#include "cc/motor.h"
#include "tests/arduino_simulator.h"

namespace markbot {
namespace {

const unsigned int ENABLE_PIN = 40;
const unsigned int DIR_PIN = 41;
const unsigned int STEP_PIN = 42;

void InitMotor(tensixty::FakeArduino &arduino, Motor *motor) {
  MotorInitProto init_proto;
  init_proto.enable_pin = ENABLE_PIN;
  init_proto.dir_pin = DIR_PIN;
  init_proto.step_pin = STEP_PIN;
  motor->Init(init_proto, &arduino);
}

void Configure(Motor *motor) {
  MotorConfigProto config;
  config.zero = true;
  config.min_steps = -1000;
  config.max_steps = 1200;
  motor->Config(config);
}

TEST(MotorTest, Init) {
  tensixty::FakeArduino arduino;
  Motor motor;
  InitMotor(arduino, &motor);
  EXPECT_EQ(motor.Direction(), false);
  EXPECT_EQ(motor.StepsRemaining(), 0);
}

TEST(MotorTest, Step) {
  tensixty::FakeArduino arduino;
  Motor motor;
  InitMotor(arduino, &motor);
  Configure(&motor);
  motor.FastTick();
  EXPECT_EQ(motor.StepsRemaining(), 0);
  EXPECT_EQ(arduino.testGetPinMode(ENABLE_PIN), tensixty::OUTPUT);
  EXPECT_EQ(arduino.testGetPinMode(DIR_PIN), tensixty::OUTPUT);
  EXPECT_EQ(arduino.testGetPinMode(STEP_PIN), tensixty::OUTPUT);
  EXPECT_EQ(arduino.testGetPinOutput(ENABLE_PIN), true);
  {
    MotorMoveProto move_proto;
    move_proto.max_speed = 1.0;
    move_proto.min_speed = 1.0;
    move_proto.acceleration = 0.001;
    move_proto.absolute_steps = 100;
    motor.Update(move_proto);
  }
  EXPECT_EQ(motor.StepsRemaining(), 100);
  EXPECT_EQ(motor.Direction(), true);
  EXPECT_EQ(arduino.testGetPinOutput(DIR_PIN), true);
  EXPECT_EQ(arduino.testGetPinOutput(ENABLE_PIN), false);
  int pin_steps = 0;
  for (int i = 0; i < 200; ++i) {
    bool step_before = arduino.testGetPinOutput(STEP_PIN);
    motor.FastTick();
    if (step_before != arduino.testGetPinOutput(STEP_PIN)) {
      ++pin_steps;
    }
    if (i < 100) {
      EXPECT_EQ(motor.StepsRemaining(), 99 - i);
    } else {
      EXPECT_EQ(motor.StepsRemaining(), 0);
    }
  }
  EXPECT_EQ(arduino.testGetPinOutput(DIR_PIN), true);
  EXPECT_EQ(pin_steps, 100);
}

TEST(MotorTest, Tare) {
  tensixty::FakeArduino arduino;
  Motor motor;
  InitMotor(arduino, &motor);
  Configure(&motor);
  motor.FastTick();
  EXPECT_EQ(motor.StepsRemaining(), 0);
  {
    MotorMoveProto move_proto;
    move_proto.max_speed = 1.0;
    move_proto.min_speed = 1.0;
    move_proto.acceleration = 0.001;
    move_proto.absolute_steps = 100;
    motor.Update(move_proto);
  }
  EXPECT_EQ(arduino.testGetPinOutput(DIR_PIN), true);
  EXPECT_EQ(motor.StepsRemaining(), 100);
  motor.Tare(-21);
  EXPECT_EQ(arduino.testGetPinOutput(DIR_PIN), true);
  EXPECT_EQ(motor.StepsRemaining(), 121);
  motor.Tare(300);
  EXPECT_EQ(motor.StepsRemaining(), 200);
}

TEST(MotorTest, ObserveMaxPositions) {
  tensixty::FakeArduino arduino;
  Motor motor;
  InitMotor(arduino, &motor);
  Configure(&motor);
  motor.FastTick();
  EXPECT_EQ(motor.StepsRemaining(), 0);
  {
    MotorMoveProto move_proto;
    move_proto.max_speed = 1.0;
    move_proto.min_speed = 1.0;
    move_proto.acceleration = 0.001;
    move_proto.absolute_steps = 2000;
    motor.Update(move_proto);
  }
  EXPECT_EQ(motor.StepsRemaining(), 1200);
  {
    MotorMoveProto move_proto;
    move_proto.max_speed = 1.0;
    move_proto.min_speed = 1.0;
    move_proto.acceleration = 0.001;
    move_proto.absolute_steps = -2000;
    motor.Update(move_proto);
  }
  EXPECT_EQ(motor.StepsRemaining(), 1000);
}

TEST(MotorTest, SpeedUpAtStart) {
  tensixty::FakeArduino arduino;
  Motor motor;
  InitMotor(arduino, &motor);
  {
    MotorConfigProto config;
    config.zero = true;
    config.min_steps = -1000;
    config.max_steps = 12000;
    motor.Config(config);
  }
  {
    MotorMoveProto move_proto;
    move_proto.max_speed = 1.0;
    move_proto.min_speed = 0.0;
    move_proto.acceleration = 0.001;
    move_proto.absolute_steps = 10000;
    motor.Update(move_proto);
  }
  motor.FastTick();
  EXPECT_EQ(motor.StepsRemaining(), 10000);
  motor.FastTick();
  EXPECT_EQ(motor.StepsRemaining(), 10000);
  for (int i = 0; i < 42; ++i) {
    motor.FastTick();
    EXPECT_EQ(motor.StepsRemaining(), 10000);
  }
  motor.FastTick();
  EXPECT_EQ(motor.StepsRemaining(), 9999);
  for (int i = 0; i < 17; ++i) {
    motor.FastTick();
    EXPECT_EQ(motor.StepsRemaining(), 9999);
  }
  motor.FastTick();
  for (int i = 0; i < 14; ++i) {
    motor.FastTick();
    EXPECT_EQ(motor.StepsRemaining(), 9998);
  }
  motor.FastTick();
  EXPECT_EQ(motor.StepsRemaining(), 9997);
  for (int i = 0; i < 11; ++i) {
    motor.FastTick();
    EXPECT_EQ(motor.StepsRemaining(), 9997);
  }
  motor.FastTick();
  EXPECT_EQ(motor.StepsRemaining(), 9996);
}

TEST(MotorTest, ReachMaxSpeedAndSlowBackDown) {
  tensixty::FakeArduino arduino;
  Motor motor;
  InitMotor(arduino, &motor);
  Configure(&motor);
  {
    MotorMoveProto move_proto;
    move_proto.max_speed = 1.0;
    move_proto.min_speed = 0.0;
    move_proto.acceleration = 0.01;
    move_proto.absolute_steps = 1000;
    motor.Update(move_proto);
  }
  bool reached_max_speed = false;
  int tick_of_first_full_speed = 0;
  int steps_of_first_full_speed = 0;
  int steps_of_last_full_speed = 0;
  for (int i = 0; i < 1200; ++i) {
    if (i < 99) {
      EXPECT_LT(motor.speed(), 1.0);
    }
    motor.FastTick();
    if (motor.StepsRemaining()) {
      ASSERT_GT(motor.speed(), 0.0) << "Zero speed with "
        << motor.StepsRemaining() << " steps left.";
    }
    if (!reached_max_speed) {
      reached_max_speed = motor.speed() == 1.0;
    }
    if (reached_max_speed && tick_of_first_full_speed == 0) {
      tick_of_first_full_speed = i;
      steps_of_first_full_speed = motor.StepsRemaining();
    }
    if (motor.speed() == 1.0) {
      steps_of_last_full_speed = motor.StepsRemaining();
    }
    EXPECT_EQ(arduino.testGetPinOutput(DIR_PIN), true);
  }
  EXPECT_TRUE(reached_max_speed);
  EXPECT_GT(steps_of_first_full_speed, 0);
  EXPECT_GT(steps_of_last_full_speed, 0);
  EXPECT_EQ(steps_of_last_full_speed, 999 - steps_of_first_full_speed);
  EXPECT_EQ(motor.StepsRemaining(), 0);
  EXPECT_LT(motor.speed(), 0.1);
}

TEST(MotorTest, ReachMaxSpeedAndSlowBackDownNegativeAndOffset) {
  tensixty::FakeArduino arduino;
  Motor motor;
  InitMotor(arduino, &motor);
  Configure(&motor);
  motor.Tare(200);
  {
    MotorMoveProto move_proto;
    move_proto.max_speed = 1.0;
    move_proto.min_speed = 0.0;
    move_proto.acceleration = 0.01;
    move_proto.absolute_steps = -800;
    motor.Update(move_proto);
  }
  bool reached_max_speed = false;
  int tick_of_first_full_speed = 0;
  int steps_of_first_full_speed = 0;
  int steps_of_last_full_speed = 0;
  for (int i = 0; i < 1200; ++i) {
    if (i < 99) {
      EXPECT_LT(motor.speed(), 1.0);
    }
    motor.FastTick();
    if (motor.StepsRemaining()) {
      EXPECT_GT(motor.speed(), 0.0);
    }
    if (!reached_max_speed) {
      reached_max_speed = motor.speed() == 1.0;
    }
    if (reached_max_speed && tick_of_first_full_speed == 0) {
      tick_of_first_full_speed = i;
      steps_of_first_full_speed = motor.StepsRemaining();
    }
    if (motor.speed() == 1.0) {
      steps_of_last_full_speed = motor.StepsRemaining();
    }
    EXPECT_EQ(arduino.testGetPinOutput(DIR_PIN), false);
  }
  EXPECT_TRUE(reached_max_speed);
  EXPECT_GT(steps_of_first_full_speed, 0);
  EXPECT_GT(steps_of_last_full_speed, 0);
  EXPECT_EQ(steps_of_last_full_speed, 999 - steps_of_first_full_speed);
  EXPECT_EQ(motor.StepsRemaining(), 0);
  EXPECT_LT(motor.speed(), 0.1);
}

TEST(MotorTest, TriangleRamp) {
  tensixty::FakeArduino arduino;
  Motor motor;
  InitMotor(arduino, &motor);
  Configure(&motor);
  {
    MotorMoveProto move_proto;
    move_proto.max_speed = 1.0;
    move_proto.min_speed = 0.0;
    move_proto.acceleration = 0.005;
    move_proto.absolute_steps = 90;
    motor.Update(move_proto);
  }
  float max_speed = 0.0;
  int steps_at_max_speed = 0;
  for (int i = 0; i < 1200; ++i) {
    EXPECT_LT(motor.speed(), 1.0);
    motor.FastTick();
    if (motor.StepsRemaining()) {
      EXPECT_GT(motor.speed(), 0.0);
    }
    if (motor.speed() > max_speed) {
      max_speed = motor.speed();
      steps_at_max_speed = motor.StepsRemaining();
    }
  }
  EXPECT_EQ(steps_at_max_speed, 44);
  EXPECT_EQ(motor.StepsRemaining(), 0);
  EXPECT_LT(motor.speed(), 0.2);
}

TEST(MotorTest, TriangleRampNegative) {
  tensixty::FakeArduino arduino;
  Motor motor;
  InitMotor(arduino, &motor);
  Configure(&motor);
  {
    MotorMoveProto move_proto;
    move_proto.max_speed = 1.0;
    move_proto.min_speed = 0.0;
    move_proto.acceleration = 0.005;
    move_proto.absolute_steps = -90;
    motor.Update(move_proto);
  }
  float max_speed = 0.0;
  int steps_at_max_speed = 0;
  for (int i = 0; i < 1200; ++i) {
    EXPECT_LT(motor.speed(), 1.0);
    motor.FastTick();
    if (motor.StepsRemaining()) {
      EXPECT_GT(motor.speed(), 0.0);
    }
    if (motor.speed() > max_speed) {
      max_speed = motor.speed();
      steps_at_max_speed = motor.StepsRemaining();
    }
  }
  EXPECT_EQ(steps_at_max_speed, 44);
  EXPECT_EQ(motor.StepsRemaining(), 0);
  EXPECT_LT(motor.speed(), 0.2);
}

TEST(MotorTest, TareWhileMoving) {
  tensixty::FakeArduino arduino;
  Motor motor;
  InitMotor(arduino, &motor);
  Configure(&motor);
  {
    MotorMoveProto move_proto;
    move_proto.max_speed = 1.0;
    move_proto.min_speed = 0.0;
    move_proto.acceleration = 0.01;
    move_proto.absolute_steps = 90;
    motor.Update(move_proto);
  }
  while (motor.StepsRemaining() > 60) {
    motor.FastTick();
    EXPECT_LT(motor.speed(), 1.0);
    EXPECT_GT(motor.speed(), 0.0);
  }
  EXPECT_EQ(arduino.testGetPinOutput(DIR_PIN), true);
  motor.Tare(-200);  // Still trying to get to 90 in absolute coords.
  EXPECT_EQ(motor.StepsRemaining(), 290);
  EXPECT_EQ(arduino.testGetPinOutput(DIR_PIN), true);
  const float midpoint_speed = motor.speed();
  motor.FastTick();
  EXPECT_GT(motor.speed(), midpoint_speed);
  bool reached_max_speed = false;
  int slowdown_step = 1000;
  while (motor.StepsRemaining() > 0) {
    ASSERT_GT(motor.speed(), 0.0);
    motor.FastTick();
    if (motor.speed() == 1.0) {
      reached_max_speed = true;
    } else if (reached_max_speed && slowdown_step == 1000) {
      slowdown_step = motor.StepsRemaining();
    }
  }
  EXPECT_EQ(motor.StepsRemaining(), 0);
  EXPECT_EQ(slowdown_step, 35);
  EXPECT_LT(motor.speed(), 0.1);
  EXPECT_TRUE(reached_max_speed);
}

TEST(MotorTest, TareWhileMovingStop) {
  tensixty::FakeArduino arduino;
  Motor motor;
  InitMotor(arduino, &motor);
  Configure(&motor);
  {
    MotorMoveProto move_proto;
    move_proto.max_speed = 1.0;
    move_proto.min_speed = 0.0;
    move_proto.acceleration = 0.01;
    move_proto.absolute_steps = 90;
    motor.Update(move_proto);
  }
  while (motor.StepsRemaining() > 60) {
    motor.FastTick();
    EXPECT_LT(motor.speed(), 1.0);
    EXPECT_GT(motor.speed(), 0.0);
  }
  motor.Tare(90);  // Still trying to get to 90 in absolute coords.
  EXPECT_EQ(motor.StepsRemaining(), 0);
  motor.FastTick();
  EXPECT_EQ(motor.StepsRemaining(), 0);
  EXPECT_EQ(motor.speed(), 0.0);
}

TEST(MotorTest, TareReverse) {
  tensixty::FakeArduino arduino;
  Motor motor;
  InitMotor(arduino, &motor);
  Configure(&motor);
  {
    MotorMoveProto move_proto;
    move_proto.max_speed = 1.0;
    move_proto.min_speed = 0.0;
    move_proto.acceleration = 0.01;
    move_proto.absolute_steps = 90;
    motor.Update(move_proto);
  }
  EXPECT_TRUE(motor.Direction());
  while (motor.StepsRemaining() > 60) {
    motor.FastTick();
    EXPECT_LT(motor.speed(), 1.0);
    EXPECT_GT(motor.speed(), 0.0);
  }
  EXPECT_EQ(arduino.testGetPinOutput(DIR_PIN), true);
  motor.Tare(100);
  EXPECT_EQ(arduino.testGetPinOutput(DIR_PIN), false);
  EXPECT_FALSE(motor.Direction());
  EXPECT_EQ(motor.StepsRemaining(), 10);
  while (motor.StepsRemaining() > 0) {
    motor.FastTick();
    EXPECT_LT(motor.speed(), 1.0);
    ASSERT_GT(motor.speed(), 0.0);
  }
  EXPECT_LT(motor.speed(), 0.21);
}

TEST(MotorTest, DisableAfterMoving) {
  tensixty::FakeArduino arduino;
  Motor motor;
  InitMotor(arduino, &motor);
  Configure(&motor);
  EXPECT_EQ(arduino.testGetPinOutput(ENABLE_PIN), true);
  {
    MotorMoveProto move_proto;
    move_proto.max_speed = 1.0;
    move_proto.min_speed = 0.0;
    move_proto.acceleration = 0.01;
    move_proto.absolute_steps = 90;
    move_proto.disable_after_moving = true;
    motor.Update(move_proto);
  }
  while (motor.StepsRemaining() > 0) {
    EXPECT_EQ(arduino.testGetPinOutput(ENABLE_PIN), false);
    motor.FastTick();
    EXPECT_GT(motor.speed(), 0.0);
  }
  motor.FastTick();
  EXPECT_EQ(arduino.testGetPinOutput(ENABLE_PIN), true);
}

TEST(MotorTest, TareIf) {
  tensixty::FakeArduino arduino;
  Motor motor;
  InitMotor(arduino, &motor);
  Configure(&motor);
  EXPECT_EQ(motor.current_position(), 0);
  MotorTareIfProto tare_if;
  tare_if.address = 0;
  tare_if.tare_rule_index = 0;
  tare_if.tare_to_steps = -42;
  tare_if.pin_to_watch = 3;
  tare_if.pin_state_to_match = true;
  motor.TareIf(tare_if);
  EXPECT_EQ(motor.current_position(), 0);
  motor.MaybeTare(0x00);
  EXPECT_EQ(motor.current_position(), 0);
  motor.MaybeTare(0x01);
  EXPECT_EQ(motor.current_position(), 0);
  motor.MaybeTare(0x08);
  EXPECT_EQ(motor.current_position(), -42);
  EXPECT_EQ(motor.StepsRemaining(), 42);

  // Try matching a low pin
  tare_if.pin_state_to_match = false;
  tare_if.tare_to_steps = 99;
  motor.TareIf(tare_if);
  motor.MaybeTare(0xFF);
  EXPECT_EQ(motor.current_position(), -42);
  motor.MaybeTare(0xF7);
  EXPECT_EQ(motor.current_position(), 99);
  EXPECT_EQ(motor.StepsRemaining(), 99);

  // Try adding another tare
  tare_if.tare_rule_index = 1;
  tare_if.pin_to_watch = 4;
  tare_if.pin_state_to_match = true;
  tare_if.tare_to_steps = -3;
  motor.TareIf(tare_if);
  motor.MaybeTare(0x08);
  EXPECT_EQ(motor.current_position(), 99);
  motor.MaybeTare(0x18);
  EXPECT_EQ(motor.current_position(), -3);
  EXPECT_EQ(motor.StepsRemaining(), 3);
}

}  // namespace
}  // namespace markbot
