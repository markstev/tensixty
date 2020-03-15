import unittest
from hardware_abstraction import QueueSerialConnection, FaultableSerialConnection
import logging
from commlink import Packet, RXThread
from queue import Queue
import time
from profile import Profile

logger = logging.getLogger()
logger.level = logging.DEBUG


def ToInts(command):
    return [ord(i) for i in command]


class ProfileTest(unittest.TestCase):
    def setUp(self):
        self.serial_connection = QueueSerialConnection('profile_test', 9600)
        self.thread_a = RXThread(self.serial_connection, 'A')
        self.thread_b = RXThread(
                self.serial_connection.MakePair(),
                '                                                   B')

    def test_profile(self):
        logging.info("--- profile ---")
        self.thread_a.start()
        self.thread_b.start()
        def run_comms(iters):
            for i in range(iters):
                logging.info("Iteration %d", i)
                self.thread_a.WriteMessage(ToInts('hello') + [i])
                self.thread_b.WriteMessage(ToInts('world!') + [i])
                m0 = self.thread_b.ReadMessage(0.1)
                m1 = self.thread_a.ReadMessage(0.1)
                self.assertEqual(m0, ToInts('hello') + [i])
                self.assertEqual(m1, ToInts('world!') + [i])
        prof = Profile()
        prof.runcall(run_comms, 20)
        prof.print_stats()


if __name__ == '__main__':
        unittest.main()
