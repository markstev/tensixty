import unittest
import hardware_abstraction
import logging
from commlink import Packet

logger = logging.getLogger()
logger.level = logging.DEBUG

class PacketTest(unittest.TestCase):
    def setUp(self):
        pass

    def testSerializeUnserialize(self):
        p = (Packet()
                .WithData(3, list(range(4)))
                .WithAck(0, False)
                )
        ser = p.SerializeToInts()
        self.assertGreater(len(ser), 8)
        p2 = Packet()
        stream_after_read = p2.ParseFromIntStream(ser)
        self.assertEqual(stream_after_read, [])
        self.assertEqual(p2.Parsed(), True)
        self.assertEqual(p2.Index(), 3)
        self.assertEqual(p2.DataOk(), True)
        self.assertEqual(p2.index_ack, 0)
        self.assertEqual(p2.data_length, 4)
        self.assertEqual(p2.data, list(range(4)))

    def testUnserializeHeaderError(self):
        p = (Packet()
                .WithData(3, list(range(4)))
                .WithAck(0, False)
                )
        ser = p.SerializeToInts()
        self.assertGreater(len(ser), 8)
        for i in range(7):
            stream = ser
            stream[i] = (stream[i] + 7) % 256
            p2 = Packet()
            stream_after_read = p2.ParseFromIntStream(ser)
            self.assertEqual(stream_after_read, stream)
            self.assertEqual(p2.Parsed(), False)
        for i in range(7):
            stream = ser[:i] + [42] + ser[i:]
            p2 = Packet()
            stream_after_read = p2.ParseFromIntStream(ser)
            self.assertEqual(p2.Parsed(), False)
        for i in range(0, 6):
            stream = ser[:i] + ser[i + 1:]
            p2 = Packet()
            stream_after_read = p2.ParseFromIntStream(ser)
            self.assertEqual(p2.Parsed(), False)


    def testUnserializeDataError(self):
        p = (Packet()
                .WithData(3, list(range(4)))
                .WithAck(0, False)
                )
        ser = p.SerializeToInts()
        self.assertGreater(len(ser), 8)
        for i in range(7, 13):
            stream = ser
            stream[i] = (stream[i] + 7) % 256
            p2 = Packet()
            stream_after_read = p2.ParseFromIntStream(stream)
            self.assertEqual(stream_after_read, stream)
            self.assertEqual(p2.Parsed(), True)
            self.assertEqual(p2.DataOk(), False)

    def testEventuallySerialize(self):
        p = (Packet()
                .WithData(3, list(range(4)))
                .WithAck(0, False)
                )
        ser = p.SerializeToInts()
        self.assertGreater(len(ser), 8)
        for i in range(0, 13):
            full_stream = []
            parsed = False
            for j in range(256):
                logging.info("replace byte %d with %d", i, j)
                stream = ser
                stream[i] = j
                full_stream += stream
                p2 = Packet()
                stream_after_read = p2.ParseFromIntStream(full_stream)
                if p2.Parsed() and p2.DataOk():
                    break
            self.assertEqual(p2.Parsed(), True)
            self.assertEqual(p2.DataOk(), True)
            self.assertEqual([], stream_after_read)


if __name__ == '__main__':
        unittest.main()
