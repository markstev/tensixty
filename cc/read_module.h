#include "arduino.h"
#include "module_dispatcher.h"
#ifdef CMAKE_MODE
  #include "motor_command.pb.h"
#else
  #include "cc/motor_command.pb.h"
#endif

namespace markbot {

const unsigned char READ_REPORT = 0x96;
const unsigned char READ_REQUEST = 0x20;

class ReadModule : public Module {
 public:
  ReadModule(tensixty::ArduinoInterface *arduino);

  Message Tick() override;

  bool AcceptMessage(const Message &message) override;

 private:
  tensixty::ArduinoInterface *arduino_;
  int64_t pin_states_;
  int64_t pins_to_read_bitmap_;
  uint8_t report_buffer_[16];
  bool force_send_message_;
};

}  // namespace markbot
