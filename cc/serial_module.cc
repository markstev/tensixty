#include "serial_module.h"

namespace markbot {

Message SerialModule::Tick() {
  rx_tx_.Tick();
  unsigned char length;
  const unsigned char* data = rx_tx_.Receive(&length);
  return Message(length, data);
}

bool SerialModule::AcceptMessage(const Message &message) {
  if ((message.type() & SERIAL_TYPE_PREFIX) != SERIAL_TYPE_PREFIX) {
    return true;
  }
  return rx_tx_.Transmit(message.raw_data(), message.raw_length());
}

}  // namespace markbot
