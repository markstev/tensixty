#include "tests/arduino_simulator.h"
#include "cc/commlink.h"
#include <cstring>
#include <stdlib.h>
#include <stdio.h>

const unsigned char DELIMITER = '\\';
const unsigned char NEWLINE = '\n';

class DelimitParser {
 public:
  DelimitParser()
    : length_so_far_(0), delimited_(false) {}

  const unsigned char* Consume(const unsigned char byte, int *length_returned) {
    *length_returned = 0;
    if (!delimited_ && byte == DELIMITER) {
      delimited_ = true;
      return nullptr;
    }
    if (!delimited_ && byte == NEWLINE) {
      *length_returned = length_so_far_;
      length_so_far_ = 0;
      delimited_ = false;
      return buffer_;
    }
    delimited_ = false;
    buffer_[length_so_far_] = byte;
    ++length_so_far_;
    return nullptr;
  }

 private:
  unsigned char buffer_[512];
  int length_so_far_;
  bool delimited_;
};

void WriteToFile(FILE *f, const unsigned char *data, int length) {
  for (int i = 0; i < length; ++i) {
    const unsigned char byte = data[i];
    if (byte == DELIMITER || byte == NEWLINE) {
      fputc(DELIMITER, f);
    }
    fputc(byte, f);
  }
  fputc(NEWLINE, f);
  fflush(f);
}

bool ReadFile(FILE *f, unsigned char *byte) {
  int read_result = fgetc(f);
  if (read_result == EOF) {
    clearerr(f);
    return false;
  }
  *byte = read_result;
  return true;
}

int main(int argc, char **argv) {
  // arg1 = incoming serial file.
  // arg2 = outgoing serial file.
  printf("Commlink main running.\n");
  if (argc < 4) {
    printf("Call with args:\nbidir_serial_main <incoming serial filename> <outgoing serial filename> <address>");
    return 1;
  }
  tensixty::FakeArduino fake_arduino;
  fake_arduino.UseFiles(argv[4], argv[5]);
  const int address = atoi(argv[3]);
  tensixty::RxTxPair rx_tx_pair(address, *tensixty::GetRealClock(), &fake_arduino);
  DelimitParser parser;
  FILE* python_to_cc_file = fopen(argv[1], "rb+");
  FILE* cc_to_python_file = fopen(argv[2], "wb+");
  tensixty::Clock &clock = *tensixty::GetRealClock();
  unsigned long start_time = clock.micros();
  unsigned long init_time = 0;
  unsigned long rx_time = 0;
  unsigned long tx_time = 0;
  unsigned long file_read_time = 0;
  unsigned long file_write_time = 0;
  unsigned long last_print_time = clock.micros();
  while (true) {
    if (clock.micros() - last_print_time > 50000) {
      printf("Init time = %lu\nRX time = %lu\nTX time = %lu\nRead time = %lu\nWrite time = %lu\n",
          init_time, rx_time, tx_time, file_read_time, file_write_time);
      last_print_time = clock.micros();
    }
    fflush(stdout);
    rx_tx_pair.Tick();
    if (!rx_tx_pair.Initialized()) continue;
    if (init_time == 0) {
      init_time = clock.micros() - start_time;
    }
    start_time = clock.micros();
    unsigned char length;
    const unsigned char *incoming = rx_tx_pair.Receive(&length);
    rx_time += clock.micros() - start_time;
    if (length > 0) {
      //printf("%d Respond to message of length %d", address, length);
      start_time = clock.micros();
      WriteToFile(cc_to_python_file, incoming, length);
      file_write_time += clock.micros() - start_time;
    }
    while (true) {
      start_time = clock.micros();
      unsigned char byte;
      const bool got_byte = ReadFile(python_to_cc_file, &byte);
      file_read_time += clock.micros() - start_time;
      if (!got_byte) break;
      start_time = clock.micros();
      //printf("%d Read byte: %d\n", address, byte);
      int length;
      const unsigned char *msg = parser.Consume(byte, &length);
      if (length > 0) {
        rx_tx_pair.Transmit(msg, length);
      }
      tx_time += clock.micros() - start_time;
    }
  }
  return 0;
}
