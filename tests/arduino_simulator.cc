#include "arduino_simulator.h"
#include <chrono>
#include <ctime>

namespace tensixty {
namespace {
bool CreateAndCloseFile(const char *filename) {
  FILE *f = fopen(filename, "w");
  if (f == nullptr) return false;
  fclose(f);
  return true;
}

class RealClock : public Clock {
 public:
  unsigned long micros() const override {
    using std::chrono::high_resolution_clock;
    using std::chrono::duration;
    using std::chrono::duration_cast;
    const static high_resolution_clock::time_point t1 =
        high_resolution_clock::now();
    const high_resolution_clock::time_point t2 = high_resolution_clock::now();
    const duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    return static_cast<unsigned long>(1e6 * time_span.count());
  }
};

Clock* clock_in_use = nullptr;

Clock* GetClock() {
  if (clock_in_use == nullptr) {
    static RealClock real_clock;
    clock_in_use = &real_clock;
  }
  return clock_in_use;
}

}  // namespace

FakeClock::FakeClock() : current_time_micros_(0) {}
unsigned long FakeClock::micros() const {
  return current_time_micros_;
}

void FakeClock::IncrementTime(const unsigned long micros) {
  current_time_micros_ += micros;
}

FakeClock* GetFakeClock() {
  static FakeClock clock;
  clock_in_use = &clock;
  return &clock;
}


FakeArduino::~FakeArduino() {
  if (incoming_file_ != nullptr) {
    fclose(incoming_file_);
  }
  if (outgoing_file_ != nullptr) {
    fclose(outgoing_file_);
  }
}

void FakeArduino::write(const unsigned char c) {
  fputc(c, outgoing_file_);
  fflush(outgoing_file_);
  printf("Wrote %d = %02X\n", c, c);
}

unsigned char FakeArduino::read() {
  if (next_byte_ != EOF) {
    const int ret = next_byte_;
    next_byte_ = EOF;
    if (muted_) return -1;
    return ret;
  }
  int read_result = fgetc(incoming_file_);
  if (read_result == EOF) {
    clearerr(incoming_file_);
    return -1;
  }
  if (muted_) return -1;
  return read_result;
}

bool FakeArduino::available() {
  if (next_byte_ != EOF) return true;
  next_byte_ = fgetc(incoming_file_);
  if (next_byte_ != EOF) {
    return true;
  } else {
    clearerr(incoming_file_);
    return false;
  }
}

bool FakeArduino::UseFiles(const char *incoming, const char *outgoing) {
  incoming_file_ = fopen(incoming, "rb+");
  printf("Incoming serial file: %s\n", incoming);
  printf("Outgoing serial file: %s\n", outgoing);
  if (incoming_file_ == nullptr) {
    // Creates the file and tries again.
    CreateAndCloseFile(incoming);
    incoming_file_ = fopen(incoming, "rb+");
  }
  outgoing_file_ = fopen(outgoing, "wb+");
  return incoming_file_ != nullptr && outgoing_file_ != nullptr;
}

unsigned long FakeArduino::micros() const {
  return GetClock()->micros();
}

void FakeArduino::setPinModeOutput(unsigned int pin) {}
void FakeArduino::setPinModeInput(unsigned int pin) {}
void FakeArduino::setPinModePullup(unsigned int pin) {}

void FakeArduino::digitalWrite(const unsigned int pin, bool value) {
  pin_states_[pin] = value;
}

bool FakeArduino::digitalRead(const unsigned int pin) {
  return pin_states_[pin];
}

Clock* GetRealClock() {
  static RealClock clock;
  return &clock;
}

}  // namespace tensixty
