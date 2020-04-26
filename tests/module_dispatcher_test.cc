#include <gtest/gtest.h>
#include "cc/module_dispatcher.h"

namespace markbot {
namespace {

class PassAndHoldModule : public Module {
 public:
  explicit PassAndHoldModule(const unsigned char address)
    : address_(address), length_(0), type_(0) {
  }

  Message Tick() override {
    if (length_ == 0) return Message(0, nullptr);
    if (type_ == address_) return Message(0, nullptr);
    Print("Tick sending message");
    return Message(type_, length_, hold_data_);
  }

  bool AcceptMessage(const Message &message) override {
    if (message.type() != address_) {
      printf("%d does not match address %d\n", message.type(), address_);
      return true;
    } else {
      printf("Handling message: %d\n", address_);
    }
    type_ = message.type();
    memcpy(hold_data_, message.data(), message.length());
    length_ = message.length();
    Print("AcceptMessage()");
    return true;
  }

  const unsigned char *data(unsigned char *length) {
    *length = length_;
    if (length_ == 0) return nullptr;
    Print("data()");
    return hold_data_;
  }

  const unsigned char type() const { return type_; }

  void FillMessageForTest(const Message &message) {
    type_ = message.type();
    memcpy(hold_data_, message.data(), message.length());
    length_ = message.length();
    Print("FillMessageForTest()");
  }

  void Print(const char *header) {
    printf("%s Fill Message Address %d; Type %d; length %d; data = ",
           header, address_, type_, length_);
    for (int i = 0; i < length_ - 1; ++i) {
      printf("%d, ", hold_data_[i]);
    }
    printf("%d\n", hold_data_[length_ - 1]);
  }

 private:
  const unsigned char address_;
  unsigned char hold_data_[20];
  unsigned char length_;
  unsigned char type_;
};

TEST(ModuleDispatcher, ModulesTalk) {
  PassAndHoldModule *p0 = new PassAndHoldModule(0x02);
  PassAndHoldModule *p1 = new PassAndHoldModule(0x03);
  PassAndHoldModule *p2 = new PassAndHoldModule(0x04);
  ModuleDispatcher dispatcher;
  dispatcher.Add(p0);
  dispatcher.Add(p1);
  dispatcher.Add(p2);
  dispatcher.HandleLoopMessages();

  unsigned char length;
  EXPECT_EQ(nullptr, p0->data(&length));
  EXPECT_EQ(length, 0);
  EXPECT_EQ(nullptr, p1->data(&length));
  EXPECT_EQ(length, 0);
  EXPECT_EQ(nullptr, p2->data(&length));
  EXPECT_EQ(length, 0);

  unsigned char data_to_p2[2] = {0x42, 0x99};
  p0->FillMessageForTest(Message(/*type = */ 0x04, /*length = */ 2, data_to_p2));
  dispatcher.HandleLoopMessages();
  EXPECT_EQ(nullptr, p1->data(&length));
  EXPECT_NE(nullptr, p2->data(&length));
  EXPECT_EQ(length, 2);
  EXPECT_EQ(p2->type(), 0x04);
  EXPECT_EQ(p2->data(&length)[0], 0x42);
  EXPECT_EQ(p2->data(&length)[1], 0x99);
  dispatcher.HandleLoopMessages();
  EXPECT_EQ(nullptr, p1->data(&length));
  EXPECT_NE(nullptr, p2->data(&length));
}

}  // namespace
}  // namespace markbot
