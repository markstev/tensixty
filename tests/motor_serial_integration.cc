#include <gtest/gtest.h>
#include "cc/module_dispatcher.h"
#include "cc/serial_module.h"
#include "cc/motor_bank_module.h"
#include "tests/arduino_simulator.h"

namespace markbot {
namespace {

int main(int argc, char **argv) {
  tensixty::FakeArduino arduino;
  ModuleDispatcher dispatcher;
  SerialModule serial_module(arduino, &arduino);
  dispatcher.AddModule(&serial_module);
  MotorBankModule motors(&arduino);
  dispatcher.AddModule(&motors);
  while (true) {
    dispatcher.HandleLoopMessages();
  }
  return 0;
}

}  // namespace
}  // namespace markbot
