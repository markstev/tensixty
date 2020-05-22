#ifndef COM_GITHUB_MARKSTEV_MARKBOT_MOTOR_BANK_MODULE_H_
#define COM_GITHUB_MARKSTEV_MARKBOT_MOTOR_BANK_MODULE_H_

#include <stdio.h>
#include <string.h>

#include "module_dispatcher.h"
#include "cc/motor_command.pb.h"
#include "motor.h"

namespace markbot {

const int NUM_MOTORS = 6;

const unsigned char MOTOR_INIT = 0x10;
const unsigned char MOTOR_CONFIG = 0x11;
const unsigned char MOTOR_MOVE = 0x12;
const unsigned char MOTOR_TARE = 0x13;
const unsigned char MOTOR_REQUEST_REPORT = 0x14;
const unsigned char MOTOR_TARE_IF = 0x15;
const unsigned char MOTOR_REPORT = 0x95;

class MotorBankModule : public Module {
 public:
  MotorBankModule(tensixty::ArduinoInterface *arduino);

  Message Tick() override;

  bool AcceptMessage(const Message &message) override;

  Motor* motor(const int i);

  void Setup();

 private:
  tensixty::ArduinoInterface *arduino_;
  // If true, compile and send a report on the next tick.
  bool request_report_;
  uint8_t report_buffer_[160];
};

}  // namespace markbot

#endif  // COM_GITHUB_MARKSTEV_MARKBOT_MOTOR_BANK_MODULE_H_
