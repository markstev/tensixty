#ifndef arduino_h_
#define arduino_h_

#include "clock_interface.h"
#include "serial_interface.h"

namespace tensixty {

class ArduinoInterface : public SerialInterface, public Clock {
 public:
  virtual ~ArduinoInterface() {}

  virtual void digitalWrite(const unsigned int pin, bool value) = 0;
  virtual bool digitalRead(const unsigned int pin) = 0;
  virtual unsigned long micros() const override = 0;
  virtual void setPinModeOutput(unsigned int pin) = 0;
  virtual void setPinModeInput(unsigned int pin) = 0;
  virtual void setPinModePullup(unsigned int pin) = 0;

  virtual void write(const unsigned char c) override = 0;
  virtual unsigned char read() override = 0;
  virtual bool available() override = 0;
};

}  // namespace tensixty

#endif // arduino_h_
