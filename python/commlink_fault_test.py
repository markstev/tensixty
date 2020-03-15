import unittest
from hardware_abstraction import QueueSerialConnection, FaultableSerialConnection
import logging
from commlink import Packet, RXThread
from queue import Queue
import time

FORMAT = '%(asctime)-15s %(message)s'
logging.basicConfig(format=FORMAT)
logger = logging.getLogger()
logger.level = logging.DEBUG


def ToInts(command):
    return [ord(i) for i in command]


class DoubleEndedFaultTest(unittest.TestCase):
    def setUp(self):
        self.serial_connection = FaultableSerialConnection(QueueSerialConnection('fault_test', 9600), 0.05)
        self.thread_a = RXThread(self.serial_connection, 'A')
        self.thread_b = RXThread(
                self.serial_connection.MakePair(),
                '                                                   B')

    def test_with_mutations(self):
        logging.info("--- test_with_mutations ---")
        self.thread_a.start()
        self.thread_b.start()
        for i in range(2000):
            imod = i % 256
            self.thread_a.WriteMessage(ToInts('hello') + [imod])
            self.thread_b.WriteMessage(ToInts('world!') + [imod])
            while not self.thread_a.done():
                pass
            while not self.thread_b.done():
                pass
            b0 = self.thread_b.ReadMessage(0)
            a0 = self.thread_a.ReadMessage(0)
            b1 = self.thread_b.ReadMessage(0)
            a1 = self.thread_a.ReadMessage(0)
            self.assertEqual(b0, ToInts('hello') + [imod])
            self.assertEqual(a0, ToInts('world!') + [imod])
            self.assertEqual(None, b1)
            self.assertEqual(None, a1)


if __name__ == '__main__':
        unittest.main()
