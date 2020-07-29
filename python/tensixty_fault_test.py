import unittest
import logging
from tensixty import TensixtyConnection
import time
from threading import Thread, Semaphore
from random import Random
from pathlib import Path
from queue import Queue

FORMAT = '%(asctime)-15s %(message)s'
logging.basicConfig(format=FORMAT)
logger = logging.getLogger()
logger.level = logging.DEBUG


def ToInts(command):
    return [ord(i) for i in command]


class FaultableFileRelay(Thread):
    def __init__(self, filename_in, filename_out, mutation_rate, add_drop_rate, seed=42):
        Thread.__init__(self)
        Path(filename_in).touch()
        self.file_in = open(filename_in, 'rb+')
        self.file_out  = open(filename_out, 'wb+')
        self.random = Random(seed)
        self.mutation_rate = mutation_rate
        self.add_drop_rate = add_drop_rate
        self.stop_queue = Queue()
        self.daemon = True
        self.semaphore = Semaphore()

    def run(self):
        reading = False
        while True:
            result = self.file_in.read(1)
            if result == b'':
                # Avoids EOF getting set and skipping future reads.
                self.file_in.seek(self.file_in.tell())
                if reading:
                    self.semaphore.release()
                    self.file_out.flush()
                    reading = False
                if not self.stop_queue.empty():
                    return
                continue
            if not reading:
                self.semaphore.acquire()
                reading = True
            result = self.MaybeMutate(result)
            self.file_out.write(result)

    def MaybeMutate(self, byte):
        mutated = b''
        if self.random.random() < self.add_drop_rate:
            if self.random.random() < 0.5:
                # Drop
                return b''
            else:
                mutated += self.RandomByte()
        if self.random.random() < self.mutation_rate:
            mutated += self.RandomByte()
        else:
            mutated += byte
        return mutated

    def RandomByte(self):
        return bytes([self.random.randint(0, 255)])

    def StopAndJoin(self):
        self.stop_queue.put(1)
        self.join()
        self.file_in.close()
        self.file_out.close()


class FileRelayTest(unittest.TestCase):
    def setUp(self):
        self.random = Random()
        randnum = str(self.random.getrandbits(128))
        self.filename_in = '/tmp/tensixty_fault_test_in_' + randnum
        self.filename_out = '/tmp/tensixty_fault_test_out_' + randnum
        self.file_in = open(self.filename_in, 'wb+')
        Path(self.filename_out).touch()
        self.file_out = open(self.filename_out, 'rb+')

    def writeAndReadBack(self, file_relay, data):
        self.file_in.write(data)
        self.file_in.flush()
        t = time.time()
        data_read = b''
        while time.time() < t + 0.1:
            result = self.file_out.read(1)
            if result == b'':
                # Avoids EOF getting set and skipping future reads.
                self.file_out.seek(self.file_out.tell())
                # Return only if the file relay is finished moving bytes.
                if len(data_read) > 0 and file_relay.semaphore.acquire(blocking=False):
                    file_relay.semaphore.release()
                    break
                continue
            data_read += result
        return data_read

    def testFaultableFileRelayNoErrors(self):
        logging.info("--- testFaultableFileRelayNoErrors ---")
        fr = FaultableFileRelay(self.filename_in, self.filename_out, 0.0, 0.0)
        fr.start()
        data = self.writeAndReadBack(fr, b'12345')
        self.assertEqual(data, b'12345')
        fr.StopAndJoin()

    def testFaultableFileRelayMutationsOnly(self):
        logging.info("--- testFaultableFileRelayMutationsOnly ---")
        fr = FaultableFileRelay(self.filename_in, self.filename_out, 0.2, 0.0)
        fr.start()
        for original in [b'12345678', b'4372407240', b'as0df8asd0f8asdasdf90']:
            data = self.writeAndReadBack(fr, original)
            self.assertEqual(len(data), len(original))
            self.assertNotEqual(data, original)
            differences = 0
            for a, b in zip(data, original):
                if a != b:
                    differences += 1
            self.assertTrue(differences > 1)
            self.assertTrue(differences < len(original) - 3)
        fr.StopAndJoin()

    def testFaultableFileRelayAddDrop(self):
        logging.info("--- testFaultableFileRelayAddDrop ---")
        fr = FaultableFileRelay(self.filename_in, self.filename_out, 0.0, 0.1)
        fr.start()
        for original in [b'12345678', b'4372407240', b'as0df8asd0f8asdasdf90']:
            data = self.writeAndReadBack(fr, original)
            self.assertNotEqual(len(data), len(original))
            self.assertTrue(abs(len(data) - len(original)) < 4)
        fr.StopAndJoin()

    def testFaultableFileRelayAddDropCanMatch(self):
        logging.info("--- testFaultableFileRelayAddDropCanMatch ---")
        fr = FaultableFileRelay(self.filename_in, self.filename_out, 0.1, 0.1)
        fr.start()
        for original in [b'12345678', b'4372407240', b'as0df8asd0f8a']:
            matches = 0
            for i in range(30):
                data = self.writeAndReadBack(fr, original)
                if data == original:
                    matches += 1
            self.assertTrue(matches > 0)
            self.assertTrue(matches < 10)
        fr.StopAndJoin()

class TensixtyFaultTest(unittest.TestCase):
    def setUp(self):
        self.random = Random()
        randnum = str(self.random.getrandbits(128))
        self.filename_in = '/tmp/tensixty_fault_test_in_' + randnum
        self.filename_out = '/tmp/tensixty_fault_test_out_' + randnum
        self.file_in = open(self.filename_in, 'wb+')
        Path(self.filename_out).touch()
        self.file_out = open(self.filename_out, 'rb+')

    def createDevices(self, mutation_rate, add_drop_rate):
        randnum = str(self.random.getrandbits(128))
        fake_arduino_a_out = '/tmp/tensixty_fake_serial_a_out' + randnum
        fake_arduino_a_in = '/tmp/tensixty_fake_serial_a_in' + randnum
        fake_arduino_b_out = '/tmp/tensixty_fake_serial_b_out' + randnum
        fake_arduino_b_in = '/tmp/tensixty_fake_serial_b_in' + randnum
        self.file_relays = [
                FaultableFileRelay(fake_arduino_a_out, fake_arduino_a_in, mutation_rate, add_drop_rate),
                FaultableFileRelay(fake_arduino_b_out, fake_arduino_b_in, mutation_rate, add_drop_rate),
                ]
        for fr in self.file_relays:
            fr.start()
        # These should just pass data all the way through.
        # Python (this) -> cc -> cc -> original python (this)
        device0 = TensixtyConnection('0', fake_arduino_a_in, fake_arduino_b_out)
        device1 = TensixtyConnection('1', fake_arduino_b_in, fake_arduino_a_out)
        device0_outputs = []
        device1_outputs = []
        device0.RegisterCallback(42, lambda x: device0_outputs.append(x))
        device1.RegisterCallback(42, lambda x: device1_outputs.append(x))
        device0.start()
        device1.start()
        return device0, device1, device0_outputs, device1_outputs

    #ef testTensixtyWithNoBitFlips(self):
    #   device0, device1, device0_outputs, device1_outputs = self.createDevices(0.0, 0.0)
    #   iterations = 5
    #   for i in range(iterations):
    #       device0.SendInts([42, i, 2, 3])
    #       device1.SendInts([42, i, 0, 9, ord('\\'), 9, 0, ord('\n')])
    #       start_time = time.time()
    #       while time.time() - start_time < 2.5:
    #           if len(device1_outputs) > i:
    #               self.assertEqual([i, 2, 3], device1_outputs[i])
    #               break
    #       while time.time() - start_time < 2.5:
    #           if len(device0_outputs) > i:
    #               self.assertEqual([i, 0, 9, ord('\\'), 9, 0, ord('\n')], device0_outputs[i])
    #               break
    #   self.assertEqual(iterations, len(device1_outputs))

    def testTensixtyWithBitFlips(self):
        device0, device1, device0_outputs, device1_outputs = self.createDevices(0.01, 0.0)
        iterations = 50
        for i in range(iterations):
            device0.SendInts([42, i, 2, 3])
            # device1.SendInts([42, i, 0, 9, ord('\\'), 9, 0, ord('\n')])
            start_time = time.time()
            while time.time() - start_time < 15.0:
                if len(device1_outputs) > i:
                    self.assertEqual([i, 2, 3], device1_outputs[i])
                    break
            self.assertEqual(i + 1, len(device1_outputs))
            #   while time.time() - start_time < 15.0:
            #       if len(device0_outputs) > i:
            #           self.assertEqual([i, 0, 9, ord('\\'), 9, 0, ord('\n')], device0_outputs[i])
            #           break
            #   self.assertEqual(i + 1, len(device0_outputs))

if __name__ == '__main__':
        unittest.main()
