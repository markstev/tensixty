import unittest
from hardware_abstraction import QueueSerialConnection, FaultableSerialConnection
import logging
from commlink import Packet, RXThread
from queue import Queue
import time
from random import Random

FORMAT = '%(asctime)-15s %(message)s'
logging.basicConfig(format=FORMAT)
logger = logging.getLogger()
logger.level = logging.DEBUG


def ToInts(command):
    return [ord(i) for i in command]


class FaultableFileRelay(Thread):
    def __init__(self, filename_in, filename_out, mutation_rate, add_drop_rate, seed=42):
        Thread.__init__(self)
        self.file_in = open(filename_in, 'rb+')
        self.file_out  = open(filename_out, 'wb+')
        self.random = Random(seed)
        self.mutation_rate = mutation_rate
        self.add_drop_rate = add_drop_rate

    def run(self):
        while True:
            result = self.file_in.read(1)
            if result == b'':
                # Avoids EOF getting set and skipping future reads.
                self.file_in.seek(self.file_in.tell())
                continue
            result = self.MaybeMutate(result)
            self.file_out.write(result)
            self.file_out.flush()

    def MaybeMutate(self, byte):
        mutated = []
        if self.random.random() < self.add_drop_rate:
            if self.random.random() < 0.5:
                # Drop
                return b''
            else:
                mutated.append(self.RandomByte())
        if self.random.random() < self.mutation_rate:
            mutated.append(self.RandomByte())
        else:
            mutated.append(byte)
        return bytes(mutated)

    def RandomByte(self):
        return self.random.randint(0, 255)


class FileRelayTest(unittest.TestCase):
    def setUp(self):
        randnum = str(random.getrandbits(128))
        self.filename_in = '/tmp/tensixty_fault_test_in_%d' + randnum
        self.filename_out = '/tmp/tensixty_fault_test_out_%d' + randnum
        self.file_in = open(self.filename_in, 'wb+')
        self.file_out = open(self.filename_out, 'rb+')

    def writeAndReadBack(self, data):
        self.file_in.write(data)
        self.file_in.flush()
        t = time.time()
        data_read = []
        while time.time() < t + 0.1:
            result = self.file_out.read(1)
            if result == b'':
                # Avoids EOF getting set and skipping future reads.
                self.file_out.seek(self.file_out.tell())
                continue
            data_read.append(result)
        return bytes(data_read)

    def testFaultableFileRelayNoErrors(self):
        logging.info("--- test_with_mutations ---")
        fr = FaultableFileRelay(self.filename_in, self.filename_out, 0.0, 0.0)
        fr.start()
        data = writeAndReadBack(b'12345')
        self.assertEqual(data, b'12345')

    def testFaultableFileRelayMutationsOnly(self):
        logging.info("--- test_with_mutations ---")

    def testFaultableFileRelayAddDrop(self):
        logging.info("--- test_with_mutations ---")


if __name__ == '__main__':
        unittest.main()
