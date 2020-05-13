#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif 
#include "real_arduino.h"

namespace tensixty {

RealArduino::RealArduino() {
}

void RealArduino::digitalWrite(const unsigned int pin, bool value) {
  return ::digitalWrite(pin, value);
}

bool RealArduino::digitalRead(const unsigned int pin) {
  return ::digitalRead(pin);
}

void RealArduino::setPinModeOutput(unsigned int pin) {
  pinMode(pin, OUTPUT);
}

void RealArduino::setPinModeInput(unsigned int pin) {
  pinMode(pin, INPUT);
}

void RealArduino::setPinModePullup(unsigned int pin) {
  pinMode(pin, INPUT_PULLUP);
}

unsigned long RealArduino::micros() const {
  return ::micros();
}

void RealArduino::write(const unsigned char c) {
  Serial.write(c);
}

unsigned char RealArduino::read() {
  const unsigned char data = (unsigned char) (Serial.read() & 0xff);
  return data;
}

bool RealArduino::available() {
  return Serial.available();
}

}  // namespace tensixty
