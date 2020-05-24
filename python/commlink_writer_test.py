import unittest
import hardware_abstraction
import logging
from commlink import Ack, Packet, Writer, Reader
from queue import Queue

logger = logging.getLogger()
logger.level = logging.DEBUG

def To128(x):
    return x % 127 + 1

def FakeData(i):
    # Makes some fake test data.
    return list(range(i % 20 + 1))

class CommlinkWriterTest(unittest.TestCase):
    def setUp(self):
        self.tx = hardware_abstraction.QueueSerialConnection('ha_test', 9600)
        self.rx = self.tx.MakePair()

    def ReadIntoBuffer(self):
        buf = []
        while True:
            byte = self.rx.read()
            if byte is None:
                break
            buf.append(byte)
        return buf

    def Initialize(self, writer):
        initialized = False
        for i in range(100):
            # Might need to flush some old junk.
            buf = self.ReadIntoBuffer()
            p = Packet()
            p.ParseFromIntStream(buf)
            self.assertEqual(p.Parsed(), True, "Failed read %d" % i)
            if not p.DataOk():
                continue
            if p.index_sending != 0x80:
                continue
            self.assertEqual(len(p.data), 0)
            writer.Write(Ack(0, True), Ack(0x80, True), Queue(10))
            initialized = True
            break
        self.assertTrue(initialized)

    def testWrite(self):
        logging.info("====== testWrite ======")
        writer = Writer(self.tx)
        self.assertEqual(writer.sequence_started, False)
        self.Initialize(writer)
        self.assertEqual(writer.sequence_started, True)
        tx_queue = Queue(10)
        last_index = 0
        for i in range(1000):
            data = FakeData(i)
            tx_queue.put(data)
            writer.Write(Ack(42, True), Ack(last_index, True), tx_queue)
            buf = self.ReadIntoBuffer()
            p = Packet()
            p.ParseFromIntStream(buf)
            self.assertEqual(p.Parsed(), True)
            self.assertEqual(p.DataOk(), True)
            self.assertEqual(p.index_sending, To128(i))
            last_index = p.index_sending
            self.assertEqual(p.data, data)

    def testRetryWrite(self):
        logging.info("====== testRetryWrite ======")
        writer = Writer(self.tx)
        self.Initialize(writer)
        tx_queue = Queue(10)
        last_index = 0
        for i in range(1000):
            data = FakeData(i)
            tx_queue.put(data)
            for mode in ["retry", "ok"]:
                if mode == "retry" and i == 0:
                    continue
                writer.Write(Ack(42, True), Ack(last_index, mode == "ok"), tx_queue)
                buf = self.ReadIntoBuffer()
                p = Packet()
                p.ParseFromIntStream(buf)
                self.assertEqual(p.Parsed(), True)
                self.assertEqual(p.DataOk(), True)
                if mode == "retry":
                    self.assertEqual(p.index_sending, last_index, "retry of %d" % last_index)
                    self.assertEqual(p.data, last_data)
                else:
                    self.assertEqual(p.index_sending, To128(i))
                    self.assertEqual(p.data, data)
                last_index = p.index_sending
                last_data = data

    def testMisOrderedRetryWrite(self):
        logging.info("====== testMisOrderedRetryWrite ======")
        logging.info("Mis-ordered write.")
        # 1) Write some stuff (4 messages)
        # 2) Ack #1
        # 2) Request retry of #3
        # 3) Get retry of #2
        # 3) Get retry of #3
        # Design Decision: Should the writer prefer ordered-first or retry-known first?
        # ordered: if sender acks N, then messages < N must have been lost, but would need to track need to retry up to and including N
        # retry-known: if sender acks N, then retry N first. Messages are now flying out-of-order.
        # Ordered seems much simpler!
        writer = Writer(self.tx)
        self.Initialize(writer)
        tx_queue = Queue(10)
        for i in range(1, 5):
            data = FakeData(i)
            tx_queue.put(data)
        for i in range(4):
            writer.Write(Ack(42, True), Ack(1, True), tx_queue)
            buf = self.ReadIntoBuffer()
        tx_queue.put(FakeData(5))  # Shouldn't have an effect
        writer.Write(Ack(42, True), Ack(3, False), tx_queue)
        for i in range(2, 4):
            buf = self.ReadIntoBuffer()
            p = Packet()
            p.ParseFromIntStream(buf)
            self.assertEqual(p.Parsed(), True)
            self.assertEqual(p.DataOk(), True)
            self.assertEqual(p.index_sending, i)
            self.assertEqual(p.data, FakeData(i))
            writer.Write(Ack(42, True), Ack(0, False), tx_queue)

        # Same, but what if message #3 is okay
        self.ReadIntoBuffer()
        writer = Writer(self.tx)
        self.Initialize(writer)
        tx_queue = Queue(10)
        for i in range(1, 5):
            data = FakeData(i)
            tx_queue.put(data)
        for i in range(4):
            writer.Write(Ack(42, True), Ack(1, True), tx_queue)
            buf = self.ReadIntoBuffer()
        tx_queue.put(FakeData(5))  # Shouldn't have an effect until after retry
        writer.Write(Ack(42, True), Ack(3, True), tx_queue)
        for i in [2, 5]:
            buf = self.ReadIntoBuffer()
            p = Packet()
            p.ParseFromIntStream(buf)
            self.assertEqual(p.Parsed(), True)
            self.assertEqual(p.DataOk(), True)
            self.assertEqual(p.index_sending, i)
            self.assertEqual(p.data, FakeData(i))
            writer.Write(Ack(42, True), Ack(0, False), tx_queue)

    def testAckOnlyWrites(self):
        writer = Writer(self.tx)
        self.Initialize(writer)
        tx_queue = Queue(10)
        for ack in [True, False]:
            for i in range(4):
                writer.Write(Ack(i + 7, ack), Ack(1, True), tx_queue)
                buf = self.ReadIntoBuffer()
                p = Packet()
                p.ParseFromIntStream(buf)
                self.assertEqual(p.Parsed(), True)
                self.assertEqual(p.DataOk(), True)
                self.assertEqual(p.index_sending, 0)
                self.assertEqual(p.data, [])
                incoming_ack = p.IncomingAck()
                self.assertEqual(incoming_ack.index, i + 7)
                self.assertEqual(incoming_ack.ok, ack)



if __name__ == '__main__':
        unittest.main()
