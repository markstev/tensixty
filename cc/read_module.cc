#include "read_module.h"

namespace markbot {

ReadModule::ReadModule(tensixty::ArduinoInterface *arduino)
  : arduino_(arduino), pin_states_(0), pins_to_read_bitmap_(0) {}

Message ReadModule::Tick() {
  const int64_t old_state = pin_states_;
  uint64_t mask = 0x01;
  for (int pin = 1; pin < 64; ++pin, mask = mask << 1) {
    if ((pins_to_read_bitmap_ & mask) != 0x00) {
      const bool value = arduino_->digitalRead(pin);
      if (value) {
        pin_states_ |= mask;
      } else {
        pin_states_ &= ~mask;
      }
    }
  }
  if (pin_states_ == old_state) {
    return Message(0, nullptr);
  }
  IOReadProto read = IOReadProto_init_zero;
  read.pin_states_bitmap = pin_states_;
  int bytes_written;
  SERIALIZE(IOReadProto, read, report_buffer_, bytes_written);
  if (bytes_written == 0) {
    return Message(0, nullptr);
  }
  return Message(READ_REPORT, bytes_written, report_buffer_);
}

bool ReadModule::AcceptMessage(const Message &message) {
  switch (message.type()) {
    case READ_REQUEST: {
      PARSE_OR_RETURN(IOReadRequestProto, request_proto, message.data(), message.length());
      uint64_t mask = 0x01;
      for (int pin = 1; pin < 64; ++pin, mask = mask << 1) {
        if ((request_proto.pullup_pin_bitmap & mask) != 0x00) {
          arduino_->setPinModePullup(pin);
          pins_to_read_bitmap_ |= mask;
        } else if ((request_proto.input_pin_bitmap & mask) != 0x00) {
          arduino_->setPinModeInput(pin);
          pins_to_read_bitmap_ |= mask;
        }
      }
      return true;
    }
    default:
      return false;
  }
}

}  // namespace markbot
