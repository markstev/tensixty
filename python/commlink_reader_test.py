import unittest
import hardware_abstraction
import logging
from commlink import Ack, Packet, Reader
from queue import Queue

logger = logging.getLogger()
logger.level = logging.DEBUG

def To128(x):
    return x % 127 + 1

def FakeData(i):
    # Makes some fake test data.
    return list(range(i % 20 + 1))

class CommlinkReaderTest(unittest.TestCase):
    def setUp(self):
        self.tx = hardware_abstraction.QueueSerialConnection('ha_test', 9600)
        self.rx = self.tx.MakePair()
        self.rx_queue = Queue(10)

    def sendPacket(self, index=0, data=(1, 2, 3)):
        p = Packet()
        p.WithAck(42, error=False)
        p.WithData(To128(index), list(data))
        for b in p.SerializeToInts():
            self.tx.write(b)
        logging.info("Sending packet: %d", To128(index))

    def initializeSequence(self, reader):
        p = Packet()
        p.WithStartSequence()
        for b in p.SerializeToInts():
            self.tx.write(b)
        for i in range(5):
            reader.Read(self.rx_queue)
        self.assertEqual(reader.PopIncomingAck(), 0x80)
        self.assertTrue(reader.sequence_started)

    def testRead(self):
        logging.info("====== testRead ======")
        reader = Reader(self.rx)
        self.initializeSequence(reader)
        self.sendPacket()
        reader.Read(self.rx_queue)
        self.assertEqual(1, self.rx_queue.qsize())
        p = self.rx_queue.get()
        self.assertTrue(p.Parsed())
        self.assertTrue(p.DataOk())
        self.assertEqual(p.data, [1, 2, 3])

    def testReadUnaligned(self):
        logging.info("====== testReadUnaligned ======")
        reader = Reader(self.rx)
        self.initializeSequence(reader)
        p = Packet()
        p.WithAck(index=42, error=False)
        p.WithData(1, [1, 2, 3])
        for b in p.SerializeToInts():
            reader.Read(self.rx_queue)
            self.assertTrue(self.rx_queue.empty())
            self.tx.write(b)
        reader.Read(self.rx_queue)
        self.assertEqual(1, self.rx_queue.qsize())
        p = self.rx_queue.get()
        self.assertTrue(p.Parsed())
        self.assertTrue(p.DataOk())
        self.assertEqual(p.data, [1, 2, 3])

        self.assertEqual(reader.PopIncomingError(), None)
        self.assertEqual(reader.PopIncomingAck(), 1)
        self.assertEqual(reader.PopOutgoingError(), None)
        self.assertEqual(reader.PopOutgoingAck(), 42)

    def testReadWithErrors(self):
        logging.info("====== testReadWithErrors ======")
    #   reader = Reader(self.rx)
    #   def AddError(l, error):
    #       if error:
    #           return l[:7] + [l[7] + 2] + l[7:]
    #       else:
    #           return l
    #   for index, error in [(1, True), (2, True), (3, False)]:
    #       p = Packet()
    #       p.WithAck(index=42, error=False)
    #       p.WithData(index, [1, 2, 3])
    #       errors = 0
    #       acks = 0
    #       for b in AddError(p.SerializeToInts(), error):
    #           self.tx.write(b)
    #           reader.Read(self.rx_queue)
    #           acks += reader.PopIncomingAck() != None
    #           errors += reader.PopIncomingError() != None
    #           self.assertEqual(reader.PopOutgoingError(), None)
    #           self.assertTrue(reader.PopOutgoingAck() in [None, 42])
    #           self.assertTrue(self.rx_queue.empty())
    #       self.assertEqual(0, self.rx_queue.qsize())
    #       if error:
    #           self.assertEqual(1, errors)
    #           self.assertEqual(0, acks)
    #           self.assertEqual(reader.PopIncomingError(), None)
    #           self.assertEqual(reader.PopIncomingAck(), None)
    #       else:
    #           self.assertEqual(0, errors)
    #           self.assertEqual(1, acks)
    #           self.assertEqual(reader.PopIncomingError(), None)
    #           self.assertEqual(reader.PopIncomingAck(), None)
    #       self.assertEqual(reader.PopOutgoingError(), None)
    #       self.assertEqual(reader.PopOutgoingAck(), None)

    #   # Now, retry those and see if we get three outputs.
    #   errors = 0
    #   acks = 0
    #   for index in [1, 2]:
    #       p = Packet()
    #       p.WithAck(index=42, error=False)
    #       p.WithData(index, [1, 2, 3])
    #       for b in p.SerializeToInts():
    #           self.tx.write(b)
    #           reader.Read(self.rx_queue)
    #           acks += reader.PopIncomingAck() != None
    #           errors += reader.PopIncomingError() != None
    #           self.assertEqual(reader.PopOutgoingError(), None)
    #           self.assertTrue(reader.PopOutgoingAck() in [None, 42])
    #   self.assertEqual(3, self.rx_queue.qsize())
    #   self.assertEqual(0, errors)
    #   self.assertEqual(2, acks)
    #   for index in [1, 2, 3]:
    #       p = self.rx_queue.get()
    #       self.assertTrue(p.Parsed())
    #       self.assertTrue(p.DataOk())
    #       self.assertEqual(p.data, [1, 2, 3])
    #       self.assertEqual(p.index_sending, index)

    def testReadMany(self):
        logging.info("====== testReadMany ======")
        reader = Reader(self.rx)
        self.initializeSequence(reader)
        for i in range(1000):
            self.sendPacket(i)
            reader.Read(self.rx_queue)
            self.assertEqual(1, self.rx_queue.qsize(), "Missing message: %d" % To128(i))
            p = self.rx_queue.get()
            self.assertTrue(p.Parsed())
            self.assertTrue(p.DataOk())
            self.assertEqual(p.data, [1, 2, 3])
            self.assertEqual(p.index_sending, To128(i))
            self.assertEqual(reader.PopIncomingError(), None)
            self.assertEqual(reader.PopIncomingAck(), To128(i))
            self.assertEqual(reader.PopOutgoingError(), None)
            self.assertEqual(reader.PopOutgoingAck(), 42)

    def testReadManyErrors(self):
        logging.info("====== testReadManyErrors ======")
        reader = Reader(self.rx)
        self.initializeSequence(reader)
        error_mode = False
        was_error_mode = False
        out_of_order_packet_raw = None
        out_of_order_packet_index = None
        ack_index = 0
        for i in range(1000):
            if To128(i) == 126:
                # Sends an out-of-order packet ahead of time, and around the 128 entry loop point.
                out_of_order_packet_raw = int(i / 127)
                out_of_order_packet_index = out_of_order_packet_raw + 1
                self.sendPacket(out_of_order_packet_raw)
                ack_index = out_of_order_packet_index
                error_mode = True
            elif To128(i) == out_of_order_packet_index:
                logging.info("Repairing packet order for index %d = %d", To128(i), out_of_order_packet_index)
                self.sendPacket(125)
                ack_index = To128(125)
                was_error_mode = error_mode
                error_mode = False
            else:
                self.sendPacket(i)
                ack_index = To128(i)
            reader.Read(self.rx_queue)
            self.assertEqual(reader.PopIncomingError(), None)
            self.assertEqual(reader.PopIncomingAck(), ack_index)
            self.assertEqual(reader.PopOutgoingError(), None)
            self.assertEqual(reader.PopOutgoingAck(), 42)
            if not error_mode and not was_error_mode:
                self.assertEqual(1, self.rx_queue.qsize(), "Missing message: %d => %d" % (i, To128(i)))
                p = self.rx_queue.get()
                self.assertTrue(p.Parsed())
                self.assertTrue(p.DataOk())
                self.assertEqual(p.data, [1, 2, 3])
                self.assertEqual(p.index_sending, To128(i))
            elif was_error_mode:
                num_fixed = 3 + out_of_order_packet_raw
                self.assertEqual(num_fixed, self.rx_queue.qsize(), "Missing message: %d => %d" % (i, To128(i)))
                was_error_mode = False
                for j in list(range(125, 127)) + list(range(0, out_of_order_packet_index)):
                    logging.info("Getting recovered message %d", j)
                    p = self.rx_queue.get()
                    self.assertTrue(p.Parsed())
                    self.assertTrue(p.DataOk())
                    self.assertEqual(p.data, [1, 2, 3])
                    self.assertEqual(p.index_sending, To128(j), "Bad index for message %d" % j)
            else:
                # Error mode
                self.assertEqual(0, self.rx_queue.qsize(), "Unexpected message: %d => %d" % (i, To128(i)))

    def testIgnoreBeforeInit(self):
        logging.info("====== testIgnoreBeforeInit ======")
        reader = Reader(self.rx)
        self.sendPacket(0, [8, 9])
        self.sendPacket(1, [10, 11])
        self.initializeSequence(reader)
        self.sendPacket()
        reader.Read(self.rx_queue)
        self.assertEqual(1, self.rx_queue.qsize())
        p = self.rx_queue.get()
        self.assertTrue(p.Parsed())
        self.assertTrue(p.DataOk())
        self.assertEqual(p.data, [1, 2, 3])

if __name__ == '__main__':
        unittest.main()
