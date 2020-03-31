#include "arduino_simulator.h"
#include "commlink.h"
#include <cstring>
#include <stdlib.h>

int main(int argc, char **argv) {
  // arg1 = incoming serial file.
  // arg2 = outgoing serial file.
  if (argc < 4) {
    printf("Call with args:\nbidir_serial_main <incoming serial filename> <outgoing serial filename> <address>");
    return 1;
  }
  printf("Serial main running.\n");
  tensixty::FakeArduino fake_arduino;
  fake_arduino.UseFiles(argv[1], argv[2]);
  tensixty::RxTxPair rx_tx_pair(*tensixty::GetRealClock(), &fake_arduino);
  while (true) {
    rx_tx_pair.Tick();
    unsigned char length;
    const unsigned char *incoming = rx_tx_pair.Receive(&length);
    if (length > 0) {
      printf("Respond to message of length %d", length);
      unsigned char rewritten[length];
      for (int i = 0; i < length; ++i) {
        rewritten[i] = incoming[i] + 7;
      }
      rx_tx_pair.Transmit(rewritten, length);
    }
  }
  return 0;
}
