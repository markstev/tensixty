#ifndef arduino_h_
#define arduino_h_

#include "serial_interface.h"

namespace tensixty {

class ArduinoInterface : public SerialInterface {
 public:
   virtual ~ArduinoInterface() {}

  virtual void digitalWrite(const unsigned int pin, bool value) = 0;
  virtual bool digitalRead(const unsigned int pin) = 0;
  virtual unsigned long micros() = 0;

  virtual void write(const unsigned char c) override = 0;
  virtual int read() override = 0;
  virtual bool available() override = 0;
};

}  // namespace tensixty

#endif // arduino_h_
