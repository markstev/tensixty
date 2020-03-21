#ifndef serial_interface_h_
#define serial_interface_h_

namespace tensixty {

class SerialInterface {
 public:
   virtual ~SerialInterface() {}

  virtual void write(const unsigned char c) = 0;
  virtual int read() = 0;
  virtual bool available() = 0;
};

}  // namespace tensixty

#endif // serial_interface_h_
