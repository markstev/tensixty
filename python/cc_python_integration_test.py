# Tests integration between C++ and python serial modules, sending data both
# directions.
#
# The C++ side will emulate sending information back; in this case, just an
# incrementing counter. They python side will listen, with the ability to reset
# the counter to an arbitrary number.
#
# This will let us test:
# 1) C++ -> Python
# 2) Python -> C++
# 3) Overall response loop
#
# CPP address: 0
# Python address: 1

import unittest
import logging
import random
import subprocess
import time
from threading import Thread

from commlink import RXThread
from hardware_abstraction import FileSerialConnection, FaultableSerialConnection


FORMAT = '%(asctime)-15s %(message)s'
logging.basicConfig(format=FORMAT)
logger = logging.getLogger()
logger.level = logging.DEBUG


class PrintThread(Thread):
    def __init__(self, output_file):
        Thread.__init__(self)
        self.output_file = output_file
        self.daemon = True
        self.running = True

    def run(self):
        logging.info("Subprocess stdout thread started.")
        while self.running:
            for stdout_line in self.output_file:
                logging.info("Subprocess: %s", str(stdout_line))


class IntegrationTest(unittest.TestCase):
    def createSerials(self, name, error_rate=0.0):
        base_connection = FileSerialConnection(name + str(time.time()), 9600)
        self.serial_connection = FaultableSerialConnection(base_connection, error_rate, seed=1)
        self.thread = RXThread(self.serial_connection)
        filename = "/tmp/cc_py_integration_test_%f" % random.Random().random()
        self.output_file = open(filename, 'wb')
        self.cc_subprocess = subprocess.Popen([
            "cc/serial_main_for_test",
            base_connection.outgoing_filename,
            base_connection.incoming_filename,
            '0'],
            stdout=self.output_file)
        self.thread.start()
        self.print_thread = PrintThread(open(filename, 'rb'))
        self.print_thread.start()

    def tearDown(self):
        self.cc_subprocess.kill()
        self.output_file.close()
        self.print_thread.running = False
        self.print_thread.join()

    #ef testIncrement(self):
    #   self.createSerials('testCounting')
    #   m0 = self.thread.ReadMessage(1)
    #   self.assertEqual(m0, None)
    #   for i in range(400):
    #       logging.info("Trial %d", i)
    #       self.thread.WriteMessage([i % 200])
    #       m0 = self.thread.ReadMessage(1.01)
    #       self.assertTrue(m0 is not None)
    #       if m0 is not None:
    #           self.assertEqual(m0, [i % 200 + 7])

    def testIncrementWithErrors(self):
        self.createSerials('testCountingErrors', 0.05)
        m0 = self.thread.ReadMessage(1)
        self.assertEqual(m0, None)
        for i in range(400):
            logging.info("Trial %d at time %f", i, time.time())
            self.thread.WriteMessage([i % 200])
            m0 = self.thread.ReadMessage(9.01)
            self.assertTrue(m0 is not None, "missing response for trial %d" % i)
            if m0 is not None:
                self.assertEqual(m0, [i % 200 + 7], "bad response for trial %d; got message %s" % (i, m0))


if __name__ == '__main__':
        unittest.main()
