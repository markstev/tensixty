import unittest
import hardware_abstraction

class HardwareAbstractionTest(unittest.TestCase):
    def setUp(self):
        pass

    def assertSerialWorks(self, serial_connection):
        serial_pair = serial_connection.MakePair()
        self.assertEqual(serial_connection.read(), None)
        self.assertEqual(serial_pair.read(), None)
        serial_connection.write(b'z')
        self.assertEqual(serial_connection.read(), None)
        self.assertEqual(serial_pair.read(), b'z')
        self.assertEqual(serial_pair.read(), None)
        serial_pair.write(b'y')
        self.assertEqual(serial_pair.read(), None)
        self.assertEqual(serial_connection.read(), b'y')
        self.assertEqual(serial_connection.read(), None)

    def test_queue_serial(self):
        queue_serial = hardware_abstraction.QueueSerialConnection('ha_test', 9600)
        self.assertSerialWorks(queue_serial)

    def test_file_serial(self):
        file_serial = hardware_abstraction.FileSerialConnection('ha_test', 9600)
        self.assertSerialWorks(file_serial)

    def test_faultable_queue_no_fault(self):
        queue_serial = hardware_abstraction.QueueSerialConnection('ha_test', 9600)
        faultable_serial = hardware_abstraction.FaultableSerialConnection(queue_serial, 0.0)
        self.assertSerialWorks(faultable_serial)

    def test_faultable_queue_fault(self):
        queue_serial = hardware_abstraction.QueueSerialConnection('ha_test', 9600)
        faultable_serial = hardware_abstraction.FaultableSerialConnection(queue_serial, 0.1)
        serial_pair = faultable_serial.MakePair()
        def hamming2(s1, s2):
            """Calculate the Hamming distance between two bit strings"""
            assert len(s1) == len(s2)
            return sum(c1 != c2 for c1, c2 in zip(s1, s2))
        noiseless = b''
        actual = b''
        for i in [bytes([z]) for z in range(255)]:
            noiseless += i
            faultable_serial.write(i)
            actual += serial_pair.read()
        hamming_dist = hamming2(noiseless, actual)
        self.assertGreater(hamming_dist, 10)
        self.assertLess(hamming_dist, 50)


    def test_faultable_queue_data_loss(self):
        queue_serial = hardware_abstraction.QueueSerialConnection('ha_test', 9600)
        faultable_serial = hardware_abstraction.FaultableSerialConnection(queue_serial, 0.1, 0.7)
        serial_pair = faultable_serial.MakePair()
        num_read = 0
        for i in [bytes([z]) for z in range(255)]:
            faultable_serial.write(i)
            if serial_pair.read():
                num_read += 1
        self.assertGreater(num_read, 245)
        self.assertLess(num_read, 265)

if __name__ == '__main__':
        unittest.main()
