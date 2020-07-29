import unittest
from tensixty import Delimit, DelimitParser, TensixtyConnection
import random
from pathlib import Path
import time

class TensixtyTest(unittest.TestCase):
    def testDelimit(self):
        ints = [1, 2, 3, 4, ord('\n')]
        delimited = Delimit(ints)
        self.assertEqual(delimited, [1, 2, 3, 4, ord('\\'), ord('\n')])
        parser = DelimitParser()
        for count, i in enumerate(ints):
            message = parser.Consume(i)
            self.assertEqual(count < 4, message is None)
        self.assertEqual(message, ints[:4])

    def testEndToEnd(self):
        tests = [[4, 5],
                 [],
                 [ord('\\')],
                 [42, ord('\\'), ord('\\')],
                 [42, ord('\\'), ord('\\'), 3],
                 [42, ord('\n'), 3],
                 [42, ord('\\'), ord('\n'), 3],
                 ]
        parser = DelimitParser()
        for t in tests:
            print('Testing with ', t)
            serialized = Delimit(t) + [ord('\n')]
            for count, i in enumerate(serialized):
                message = parser.Consume(i)
                if message is not None:
                    break
            self.assertFalse(message is None)
            self.assertEqual(count + 1, len(serialized))
            self.assertEqual(message, t)

    def createDevices(self):
        randnum = str(random.getrandbits(128))
        fake_arduino_a = '/tmp/tensixty_fake_serial_a_' + randnum
        fake_arduino_b = '/tmp/tensixty_fake_serial_b_' + randnum
        # These should just pass data all the way through.
        # Python (this) -> cc -> cc -> original python (this)
        device0 = TensixtyConnection('0', fake_arduino_a, fake_arduino_b)
        device1 = TensixtyConnection('1', fake_arduino_b, fake_arduino_a)
        device0_outputs = []
        device1_outputs = []
        device0.RegisterCallback(42, lambda x: device0_outputs.append(x))
        device1.RegisterCallback(42, lambda x: device1_outputs.append(x))
        device0.start()
        device1.start()
        return device0, device1, device0_outputs, device1_outputs

    def testTwoWrappedModules(self):
        device0, device1, device0_outputs, device1_outputs = self.createDevices()
        device0.SendInts([42, 1, 2, 3])
        start_time = time.time()
        while time.time() - start_time < 2.5:
            if len(device1_outputs) > 0:
                self.assertEqual([1, 2, 3], device1_outputs[0])
        self.assertEqual(1, len(device1_outputs))
        self.assertEqual(0, len(device0_outputs))

    def testTwoWrappedModulesBackAndForth(self):
        device0, device1, device0_outputs, device1_outputs = self.createDevices()
        iterations = 50
        for i in range(iterations):
            device0.SendInts([42, i, 2, 3])
            device1.SendInts([42, i, 0, 9, ord('\\'), 9, 0, ord('\n')])
            start_time = time.time()
            while time.time() - start_time < 2.5:
                if len(device1_outputs) > i:
                    self.assertEqual([i, 2, 3], device1_outputs[i])
                    break
            while time.time() - start_time < 2.5:
                if len(device0_outputs) > i:
                    self.assertEqual([i, 0, 9, ord('\\'), 9, 0, ord('\n')], device0_outputs[i])
                    break
        self.assertEqual(iterations, len(device1_outputs))
        self.assertEqual(iterations, len(device0_outputs))


if __name__ == '__main__':
        unittest.main()
