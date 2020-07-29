import time
from threading import Thread
import subprocess
import random
import os
from pathlib import Path
import sys

NEWLINE = ord('\n')
DELIMITER = ord('\\')


def Delimit(ints):
    delimited_message = []
    for value in ints:
        if value == NEWLINE or value == DELIMITER:
            delimited_message.append(DELIMITER)
        delimited_message.append(value)
    return delimited_message


class DelimitParser(object):
    def __init__(self):
        self.buffer = []
        self.delimited = False

    def Consume(self, next_int):
        """Returns none, unless we hit the final delimiter, and then any buffered message is returned."""
        if not self.delimited and next_int == DELIMITER:
            self.delimited = True
            return None
        if not self.delimited and next_int == NEWLINE:
            message = self.buffer
            self.buffer = []
            self.delimited = False
            return message
        self.delimited = False
        self.buffer.append(next_int)
        return None

# Talks to a c++ subprocess via files. Messages are delimited via newline
# characters. Any existing newline characters are delimited with a '\' character.
class TensixtyConnection(Thread):
    def __init__(self, device_name='0', fake_arduino_file_a=None, fake_arduino_file_b=None):
        Thread.__init__(self)
        randnum = str(random.getrandbits(128))
        outgoing_filename = '/tmp/tensixty_python_to_cc_' + randnum
        incoming_filename = '/tmp/tensixty_cc_to_python_' + randnum
        self.stdout = open('/tmp/tensixty_stdout_' + randnum, 'wb')
        for f in (incoming_filename, outgoing_filename):
            try:
                os.remove(f)
            except FileNotFoundError:
                pass
        Path(incoming_filename).touch()
        self.incoming = open(incoming_filename, 'rb')
        self.outgoing = open(outgoing_filename, 'wb')
        self.cc_subprocess = subprocess.Popen([
            "python/commlink_main",
            outgoing_filename,
            incoming_filename,
            device_name,
            # Incoming
            fake_arduino_file_a,
            # Outgoing
            fake_arduino_file_b,
            ],
            stdout=sys.stdout,#self.stdout,
            )
        self.callbacks = {}
        self.daemon = True

    def run(self):
        self._ReadMessage()

    def __del__(self):
        print("Delete called.")
        self.cc_subprocess.kill()
        self.incoming.close()
        self.outgoing.close()
        self.stdout.close()

    def SendInts(self, list_of_ints):
        self._DelimitAndSend(list_of_ints)

    def SendProto(self, type, message):
        serialized = [type] + list(message.SerializeToString())
        self._DelimitAndSend(serialized)

    def RegisterCallback(self, message_prefix, func):
        self.callbacks[message_prefix] = func

    def _ReadMessage(self):
        parser = DelimitParser()
        while True:
            next_int = self._ReadInt()
            if next_int is None:
                time.sleep(0.001)
                continue
            message = parser.Consume(next_int)
            if message is None:
                continue
            for prefix, cb in self.callbacks.items():
                if message[0] == prefix:
                    cb(message[1:])
            continue

    def _ReadInt(self):
        result = self.incoming.read(1)
        if result == b'':
            # Avoids EOF getting set and skipping future reads.
            self.incoming.seek(self.incoming.tell())
            return None
        result = int.from_bytes(result, byteorder='big')
        return result

    def _DelimitAndSend(self, ints):
        self.outgoing.write(bytes(Delimit(ints) + [NEWLINE]))
        self.outgoing.flush()
