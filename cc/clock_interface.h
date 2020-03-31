#ifndef clock_interface_h_
#define clock_interface_h_

namespace tensixty {

// Interface for a variety of ways to check the current time, including fake clocks.
class Clock {
 public:
  virtual ~Clock() {}
  virtual unsigned long micros() const = 0;
};

}  // namespace tensixty

#endif // clock_interface_h_
