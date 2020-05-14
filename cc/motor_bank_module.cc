#include "motor_bank_module.h"

namespace markbot {
Motor MOTORS[NUM_MOTORS];
}  // namespace markbot

#if not __x86_64__
#if defined(ARDUINO) && ARDUINO >= 100
    #include "Arduino.h"
#else
    #include "WProgram.h"
#endif 
ISR(TIMER2_COMPA_vect) {
  for (int i = 0; i < markbot::NUM_MOTORS; ++i) {
    markbot::MOTORS[i].FastTick();
  }
}
#endif  // not __x86_64__

namespace markbot {

MotorBankModule::MotorBankModule(tensixty::ArduinoInterface *arduino)
    : arduino_(arduino), request_report_(false) {
}

void MotorBankModule::Setup() {
#if not __x86_64__
  cli();//stop interrupts

  //set timer2 interrupt at 8kHz
  TCCR2A = 0;// set entire TCCR2A register to 0
  TCCR2B = 0;// same for TCCR2B
  TCNT2  = 0;//initialize counter value to 0
  // set compare match register for 8khz increments
  // interrupt frequency = 16Mhz / prescale / OCR2A
  // Note, the physical limits of stepper motors seems to be around 4kHz.
  OCR2A = 124;  // = (16*10^6) / (4000*32) - 1
  // turn on CTC mode
  TCCR2A |= (1 << WGM21);
  // Set CS21 bit for 8 prescaler
  TCCR2B |= (1 << CS21);   
  // Set CS20 bit as well -> 32 prescaler
  TCCR2B |= (1 << CS20);   
  // enable timer compare interrupt
  TIMSK2 |= (1 << OCIE2A);
  sei();
#endif  // not __x86_64__
}

Message MotorBankModule::Tick() {
  if (request_report_) {
    request_report_ = false;
    AllMotorReportProto report = AllMotorReportProto_init_zero;
    for (int i = 0; i < NUM_MOTORS; ++i) {
      report.motors[i].current_absolute_steps = MOTORS[i].current_position();
      report.motors[i].acceleration = MOTORS[i].acceleration();
      report.motors[i].step_progress = MOTORS[i].step_progress();
      report.motors[i].step_speed = MOTORS[i].step_speed();
    }
    int bytes_written;
    SERIALIZE(AllMotorReportProto, report, report_buffer_, bytes_written);
    if (bytes_written > 0) {
      return Message(MOTOR_REPORT, bytes_written, report_buffer_);
    }
  }
  return Message(0, nullptr);
}

bool MotorBankModule::AcceptMessage(const Message &message) {
  switch (message.type()) {
    case MOTOR_INIT: {
      PARSE_OR_RETURN(MotorInitProto, init_proto, message.data(), message.length());
      MOTORS[init_proto.address].Init(init_proto, arduino_);
      break;
    }
    case MOTOR_CONFIG: {
      PARSE_OR_RETURN(MotorConfigProto, config_proto, message.data(), message.length());
      MOTORS[config_proto.address].Config(config_proto);
      break;
    }
    case MOTOR_MOVE: {
      PARSE_OR_RETURN(MotorMoveAllProto, move_proto, message.data(), message.length());
      for (int i = 0; i < NUM_MOTORS; ++i) {
        MOTORS[i].Update(move_proto.motors[i]);
      }
      break;
    }
    case MOTOR_TARE: {
      PARSE_OR_RETURN(MotorTareProto, tare_proto, message.data(), message.length());
      MOTORS[tare_proto.address].Tare(tare_proto.tare_to_steps);
      break;
    }
    case MOTOR_REQUEST_REPORT: {
      request_report_ = true;
      break;
    }
    default:
      return false;
  }
  return true;
}

Motor* MotorBankModule::motor(const int i) {
  return &MOTORS[i];
}

}  // namespace markbot
