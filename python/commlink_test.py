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


class DoubleEndedTest(unittest.TestCase):
    def setUp(self):
        self.serial_connection = QueueSerialConnection('create', 9600)
        self.thread_a = RXThread(self.serial_connection, name="A")
        self.thread_b = RXThread(self.serial_connection.MakePair(),
                name="                                              B")

    def test_send_to_each_other(self):
        logging.info("--- test_send_to_each_other  ---")
        self.thread_a.start()
        self.thread_b.start()
        self.thread_a.WriteMessage(ToInts('hello'))
        self.thread_b.WriteMessage(ToInts('world!'))
        while not self.thread_a.done() or not self.thread_b.done():
            pass
        logging.info("Should be done.")
        m0 = self.thread_b.ReadMessage(0)
        m1 = self.thread_a.ReadMessage(0)
        m2 = self.thread_b.ReadMessage(0)
        m3 = self.thread_a.ReadMessage(0)
        self.assertEqual(m0, ToInts('hello'))
        self.assertEqual(m1, ToInts('world!'))
        self.assertEqual(None, m2)
        self.assertEqual(None, m3)


    def test_send_to_each_other_timing_tweaks(self):
        # TODO: determine if getting deadlocked. Theoretically, R & W need to be in separate threads.
        logging.info("--- test_send_to_each_other_timing_tweaks  ---")
        self.thread_a.start()
        time.sleep(0.3)
        self.thread_b.start()
        self.thread_a.WriteMessage(ToInts('hello'))
        time.sleep(0.1)
        self.thread_b.WriteMessage(ToInts('world!'))
        self.thread_a.WriteMessage(ToInts('m2'))
        self.thread_b.WriteMessage(ToInts('m3'))
        m0 = self.thread_b.ReadMessage(2.9)
        m1 = self.thread_a.ReadMessage(2.9)
        m2 = self.thread_b.ReadMessage(2.9)
        m3 = self.thread_a.ReadMessage(2.9)
        self.assertEqual(m0, ToInts('hello'))
        self.assertEqual(m1, ToInts('world!'))
        self.assertEqual(m2, ToInts('m2'))
        self.assertEqual(m3, ToInts('m3'))

    def test_many_messages(self):
        logging.info("--- test_many_messages ---")
        self.thread_a.start()
        self.thread_b.start()
        for i in range(300):
            self.thread_a.WriteMessage(ToInts('hello') + [i])
            self.thread_b.WriteMessage(ToInts('world!') + [i])
            while not self.thread_a.done() or not self.thread_b.done():
                pass
            b0 = self.thread_b.ReadMessage(0.0)
            a0 = self.thread_a.ReadMessage(0.0)
            b1 = self.thread_b.ReadMessage(0.0)
            a1 = self.thread_a.ReadMessage(0.0)
            self.assertEqual(b0, ToInts('hello') + [i])
            self.assertEqual(a0, ToInts('world!') + [i])
            self.assertEqual(None, b1)
            self.assertEqual(None, a1)


if __name__ == '__main__':
        unittest.main()
