#ifndef hardware_simulator_arduino_simulator_h_
#define hardware_simulator_arduino_simulator_h_

#include "cc/arduino.h"
#include "cc/clock_interface.h"
#include <stdio.h>
#include <chrono>
#include <map>

namespace tensixty {

// By default, a real clock is used. To use a fake clock instead, call
// UseFakeClock() or GetFakeClock().
class FakeClock : public Clock {
 public:
  FakeClock();
  unsigned long micros() const override;
  void IncrementTime(const unsigned long micros);

 private:
  unsigned long current_time_micros_;
};

FakeClock* GetFakeClock();
Clock* GetRealClock();

class FakeArduino : public ArduinoInterface {
 public:
   ~FakeArduino();

  void digitalWrite(const unsigned int pin, bool value) override;
  bool digitalRead(const unsigned int pin) override;
  unsigned long micros() const override;

  void write(const unsigned char c) override;
  unsigned char read() override;
  bool available() override;
  void setPinModeOutput(unsigned int pin) override;
  void setPinModeInput(unsigned int pin) override;
  void setPinModePullup(unsigned int pin) override;

  bool UseFiles(const char *incoming, const char *outgoing);

  void Mute() {
    muted_ = true;
    printf("Muted.\n");
  }
  void UnMute() { muted_ = false; }

 private:
  FILE *incoming_file_ = nullptr;
  FILE *outgoing_file_ = nullptr;
  int next_byte_ = EOF;
  std::map<const unsigned int, bool> pin_states_;
  std::map<const unsigned int, bool> pin_modes_;
  bool muted_ = false;
};

}  // namespace tensixty

#endif
