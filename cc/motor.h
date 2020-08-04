#ifndef COM_GITHUB_MARKSTEV_MARKBOT_MOTOR_H_
#define COM_GITHUB_MARKSTEV_MARKBOT_MOTOR_H_

#ifdef CMAKE_MODE
  #include "motor_command.pb.h"
#else
  #include "cc/motor_command.pb.h"
#endif
#include <stdint.h>
#include "arduino.h"

namespace markbot {

struct TareRule {
  bool active = false;
  int32_t tare_to_steps;
  int64_t match_mask;
  bool pin_state_to_match;
};

class Motor {
 public:
  Motor();
  
  void Init(const MotorInitProto &init_proto, tensixty::ArduinoInterface *arduino);

  uint32_t address() const;

  void Update(const MotorMoveProto &move_proto);

  bool Direction() const;

  uint32_t StepsRemaining() const;

  // Used for ISR mode.
  void FastTick();

  bool MaybeDisableMotor();

  bool Step();

  void Config(const MotorConfigProto &config);

  void Tare(const int32_t tare_to_steps);
  void TareIf(const MotorTareIfProto &tare_if);
  void MaybeTare(const int64_t pin_states_bitmap);

  float speed() const { return step_speed_; }

  int32_t current_position() const { return current_absolute_steps_; }
  float acceleration() const { return acceleration_; }
  float step_progress() const { return step_progress_; }
  float step_speed() const { return step_speed_; }

 private:
  void UpdateRamps();
  tensixty::ArduinoInterface *arduino_;
  MotorInitProto init_proto_;

  // Stepping state
  bool pulse_state_;

  float min_speed_;
  float max_speed_;
  volatile float acceleration_;
  volatile float step_speed_;
  volatile float step_progress_;
  int32_t start_slowdown_step_;

  bool disable_after_moving_;
  int32_t max_steps_;
  int32_t min_steps_;
  volatile int32_t current_absolute_steps_;
  volatile int32_t target_absolute_steps_;
  int32_t increment_ = 1;
  TareRule tare_rules_[3];
};

}  // namespace markbot

#endif  // COM_GITHUB_MARKSTEV_MARKBOT_MOTOR_H_
