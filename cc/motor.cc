#include "motor.h"
#include <math.h>
#if __x86_64__
#include <algorithm>
using std::min;
using std::max;
using std::abs;
#else
#if defined(ARDUINO) && ARDUINO >= 100
    #include "Arduino.h"
#else
    #include "WProgram.h"
#endif 
#endif

namespace markbot {

Motor::Motor() {
  pulse_state_ = false;
  current_absolute_steps_ = 0;
  target_absolute_steps_ = 0;
  acceleration_ = 0.0;
  step_speed_ = 0.0;
  step_progress_ = 0.0;
}

void Motor::Init(const MotorInitProto &init_proto,
    tensixty::ArduinoInterface *arduino) {
  arduino_ = arduino;
  init_proto_ = init_proto;
  arduino_->setPinModeOutput(init_proto_.enable_pin);
  arduino_->setPinModeOutput(init_proto_.dir_pin);
  arduino_->setPinModeOutput(init_proto_.step_pin);
  arduino_->digitalWrite(init_proto_.enable_pin, true);
  current_absolute_steps_ = 0;
  // Arbitrary small number to prevent accidents
  min_steps_ = -200;
  max_steps_ = 200;
}

uint32_t Motor::address() const {
  return init_proto_.address;
}

float Min(const float a, const float b) {
  return a < b ? a : b;
}

float Abs(const float x) {
  return x < 0 ? -x : x;
}

void Motor::UpdateRamps() {
  acceleration_ = abs(acceleration_);
  step_speed_ = min_speed_;
  int decel_steps = Abs(target_absolute_steps_ - current_absolute_steps_) / 2;
  {
    // Simulate ramp times.
    float sim_speed = step_speed_;
    float step_progress = 0;
    int steps = 0;
    for (int ticks = 0; ticks < 1000000; ++ticks) {
      if (sim_speed > 1.0) {
        //printf("Found end of accel ramp at %d\n", steps);
        if (steps < decel_steps) {
          decel_steps = steps;
        }
        break;
      }
      sim_speed += acceleration_;
      step_progress += sim_speed;
      if (step_progress > 1.0) {
        step_progress = 0;
        ++steps;
      }
    }
  }
  // Handle rounding error.
  decel_steps -= 1;
  //printf("Steps needed to slow down: %d\n", decel_steps);

  if (target_absolute_steps_ > current_absolute_steps_) {
    start_slowdown_step_ = target_absolute_steps_ - decel_steps;
  } else {
    start_slowdown_step_ = target_absolute_steps_ + decel_steps;
  }
  //printf("Slowdown step: %d\n", start_slowdown_step_);
  //printf("Slowdown step = %d\n", start_slowdown_step_);
  step_progress_ = 0.0;
  if (target_absolute_steps_ == current_absolute_steps_) {
    step_speed_ = 0.0;
  }
  arduino_->digitalWrite(init_proto_.dir_pin, Direction());
  increment_ = Direction() ? 1 : -1;
  arduino_->digitalWrite(init_proto_.enable_pin, false);
  MaybeDisableMotor();
}

void Motor::Update(const MotorMoveProto &move_proto) {
  if (arduino_ == nullptr) return;
  target_absolute_steps_ = max(min_steps_, min(max_steps_, move_proto.absolute_steps));
  //printf("%d Move to %d\n", address(), target_absolute_steps_);
  min_speed_ = move_proto.min_speed;
  max_speed_ = move_proto.max_speed;
  acceleration_ = move_proto.acceleration;
  disable_after_moving_ = move_proto.disable_after_moving;
  UpdateRamps();
}

bool Motor::Direction() const {
  return target_absolute_steps_ > current_absolute_steps_;
}

uint32_t Motor::StepsRemaining() const {
  return abs(target_absolute_steps_ - current_absolute_steps_);
}

void Motor::FastTick() {
  if (arduino_ == nullptr) {
    return;
  }
  //printf("Motor %d is at %d with target %d\n", address(), current_absolute_steps_, target_absolute_steps_);
  if (current_absolute_steps_ == target_absolute_steps_) {
    return;
  }
  step_speed_ += acceleration_;
  printf("Update step speed to %f.\n", step_speed_);
  if (step_speed_ > max_speed_) {
    step_speed_ = max_speed_;
  } else if (step_speed_ < min_speed_) {
    step_speed_ = min_speed_;
  }
  step_progress_ += step_speed_;
  if (step_progress_ < 1.0) return;
  // A full reset yields smoother movement.
  step_progress_ = 0.0;
  Step();
  if (current_absolute_steps_ == target_absolute_steps_ &&
      disable_after_moving_) {
    arduino_->digitalWrite(init_proto_.enable_pin, true);
  } else if (current_absolute_steps_ == start_slowdown_step_) {
    acceleration_ = -acceleration_;
  }
}

bool Motor::MaybeDisableMotor() {
  const int steps_remaining = StepsRemaining();
  if (steps_remaining != 0) return false;
  if (disable_after_moving_) {
    arduino_->digitalWrite(init_proto_.enable_pin, true);
  }
  if (steps_remaining == 0) {
    return true;
  }
  return false;
}

bool Motor::Step() {
  current_absolute_steps_ += increment_;
  pulse_state_ = !pulse_state_;
  arduino_->digitalWrite(init_proto_.step_pin, pulse_state_);
  return true;
}

void Motor::Config(const MotorConfigProto &config) {
  if (config.zero) {
    current_absolute_steps_ = 0;
  }
  min_steps_ = config.min_steps;
  max_steps_ = config.max_steps;
}

void Motor::Tare(const int32_t tare_to_steps) {
  current_absolute_steps_ = tare_to_steps;
  if (arduino_ == nullptr) return;
  UpdateRamps();
  MaybeDisableMotor();
}

void Motor::TareIf(const MotorTareIfProto &tare_if){
  TareRule &tare_rule = tare_rules_[tare_if.tare_rule_index];
  tare_rule.active = true;
  tare_rule.tare_to_steps = tare_if.tare_to_steps;
  tare_rule.match_mask = 0x01 << tare_if.pin_to_watch;
  tare_rule.pin_state_to_match = tare_if.pin_state_to_match;
}

void Motor::MaybeTare(const int64_t pin_states_bitmap) {
  for (const auto& tare_rule : tare_rules_) {
    if (!tare_rule.active) continue;
    const int64_t masked = pin_states_bitmap & tare_rule.match_mask;
    const bool match = tare_rule.pin_state_to_match ?
      masked != 0 : masked == 0;
    if (match) {
      Tare(tare_rule.tare_to_steps);
    }
  }
}

}  // namespace markbot
