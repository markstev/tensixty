#ifndef COM_GITHUB_MARKSTEV_TENSIXTY_REAL_ARDUINO_H_
#define COM_GITHUB_MARKSTEV_TENSIXTY_REAL_ARDUINO_H_

#include "includes/tensixty/cc/arduino.h"

namespace tensixty {

const int NUM_PIN_BANKS = 3;

class RealArduino : public tensixty::ArduinoInterface {
 public:
  RealArduino();
  virtual ~RealArduino() {}

  void digitalWrite(const unsigned int pin, bool value) override;
  bool digitalRead(const unsigned int pin) override;
  unsigned long micros() const override;
  void setPinModeOutput(unsigned int pin) override;
  void setPinModeInput(unsigned int pin) override;
  void setPinModePullup(unsigned int pin) override;

  void write(const unsigned char c) override;
  unsigned char read() override;
  bool available() override;
};

}  // namespace tensixty

#endif  // COM_GITHUB_MARKSTEV_TENSIXTY_REAL_ARDUINO_H_
