#ifndef hardware_simulator_arduino_simulator_h_
#define hardware_simulator_arduino_simulator_h_

#include "arduino.h"
#include <stdio.h>
#include <map>

namespace tensixty {

// Interface for a variety of ways to check the current time, including fake clocks.
class Clock {
 public:
  ~Clock() {}
  virtual unsigned long micros() const = 0;
};

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

class FakeArduino : public ArduinoInterface {
 public:
   ~FakeArduino();

  void digitalWrite(const unsigned int pin, bool value) override;
  bool digitalRead(const unsigned int pin) override;
  unsigned long micros() override;

  void write(const unsigned char c) override;
  int read() override;
  bool available() override;

  bool UseFiles(const char *incoming, const char *outgoing);

 private:
  FILE *incoming_file_ = nullptr;
  FILE *outgoing_file_ = nullptr;
  int next_byte_ = EOF;
  std::map<const unsigned int, bool> pin_states_;
  std::map<const unsigned int, bool> pin_modes_;
};

}  // namespace tensixty

#endif
