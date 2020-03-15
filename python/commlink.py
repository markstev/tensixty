from threading import Thread
import logging
from collections import namedtuple, deque
from queue import Queue, Empty
import time

def Checksum(ints):
  first_sum = 0
  second_sum = 0
  for b in ints:
    first_sum = (first_sum + b) & 0xff
    second_sum = (second_sum + first_sum) & 0xff
  return [first_sum, second_sum]


def AppendChecksum(ints):
  return ints + Checksum(ints)


class Packet(object):
    def __init__(self):
        self.error = 0
        self.index_ack = 0
        self.index_sending = 0
        self.data_length = 0
        self.data = []
        # Only used when parsing.
        self.parsed = False
        self.data_ok = False

    def WithData(self, index, data):
        self.index_sending = index
        self.data = data
        self.data_length = len(data)
        return self

    def WithAck(self, index, error):
        self.index_ack = index
        self.error = error
        return self

    def SerializeToInts(self):
        error_add = 0
        if self.error:
            error_add = 128
        return AppendChecksum([10, 60, error_add + self.index_ack,
            self.index_sending, self.data_length]) + AppendChecksum(self.data)

    def IncomingAck(self):
        return Ack(self.index_ack, not self.error)

    def ParseFromIntStream(self, stream):
        """Consumes what it reads. Returns stream, minus what was consumed."""
        stream_length = len(stream)
        for i, b in enumerate(stream):
            if (b == 60 and i > 0 and stream[i - 1] == 10 and
                    stream_length - i > 5):
                # Start found.
                data_start = i + 6
                checksums = stream[i + 4:data_start]
                if checksums != Checksum(stream[i - 1:i + 4]):
                    logging.info("Bad header.")
                    return stream[i + 4:]
                # Got valid header
                self.error = stream[i + 1] > 127
                self.index_ack = stream[i + 1] % 128
                self.index_sending = stream[i + 2]
                self.data_length = stream[i + 3]
                if data_start + self.data_length + 2 <= stream_length:
                    self.parsed = True
                    checksum_start = data_start + self.data_length
                    self.data = stream[data_start:checksum_start]
                    self.data_ok = (Checksum(self.data) ==
                            stream[checksum_start:checksum_start + 2])
                    return stream[checksum_start + 2:]
        return stream

    def Parsed(self):
        """Implies we got a valid header and the requested number of bytes.

        May have DataOk() set to false.
        """
        return self.parsed

    def Index(self):
        return self.index_sending

    def DataOk(self):
        return self.data_ok


class Loggable(object):
    def __init__(self, name):
        self.name = name

    def log(self, *args):
        args_with_name = []
        args_with_name = ("%s: " + args[0], self.name) + args[1:]
        logging.info(*args_with_name)


class PacketRingBuffer(object):
    # TODO: I think this is cleaner, but haven't tested yet.
    BufferItem = namedtuple("BufferItem", ["empty", "packet"])
    def __init__(self):
        self.last_packet_index = 0
        self.buffer = deque()

    def PopPacket(self):
        if len(self.buffer) == 0:
            return None
        if self.buffer[0].empty:
            return None
        self.last_packet_index = self.buffer[0].index_sending
        return self.buffer.popleft()


    def InsertPacket(self, packet):
        if packet.index_sending == 0:
            return None
        offset = packet.index_sending - self.last_packet_index
        if offset > 0:
            self.AppendPacket(packet, offset)
        elif offset < -100 and offset > -127:
            self.AppendPacket(packet, offset + 127)
        else:
            # Bad, duplicate packet.
            pass

    def AppendPacket(self, packet, offset):
        for i in range(len(self.buffer), offset - 1):
            self.buffer.append(BufferItem(empty=True))
        self.buffer.append(BufferItem(empty=False, packet=packet))


class Reader(Loggable):
    def __init__(self, ser, name=""):
        Loggable.__init__(self, name)
        self.ser = ser
        self.bytes = []
        self.packet_lists = [[], []]
        self.incoming_ok = Queue(10)
        self.incoming_error = Queue(10)
        self.outgoing_ok = Queue(10)
        self.outgoing_error = Queue(10)
        self.last_packet_index = 0
        self.padded_to_end = False

    def Read(self, rx_queue):
        self.ReadIntoBuffer()
        if self.bytes:
            #self.log("Got some bytes")
            packet = Packet()
            self.bytes = packet.ParseFromIntStream(self.bytes)
            if packet.Parsed():
                self.log("Parsed")
                if packet.DataOk():
                    self.log("Data OK: %d, %s", packet.index_sending, packet.data)
                    self.incoming_ok.put(packet.index_sending)
                    popped = self.AppendPacket(packet)
                    if popped:
                        self.log("popped")
                        for p in popped:
                            self.log("Returning packet %d", p.index_sending)
                            rx_queue.put(p)
                else:
                    self.incoming_error.put(packet.index_sending)
                if packet.error:
                    self.outgoing_error.put(packet.index_ack)
                else:
                    self.outgoing_ok.put(packet.index_ack)

    def done(self):
        return len(self.bytes) == 0 and self.packet_lists == [[], []]

    def AppendPacket(self, packet):
        if packet.index_sending == 0:
            return None
        offset = packet.index_sending - self.last_packet_index - 1
        self.log("Offset = %d", offset)
        if offset < -100:
            packets = self.packet_lists[1]
            offset = packet.index_sending - 1
            if not self.padded_to_end:
                for i in range(127 - self.last_packet_index):
                    # avoids this will get hit many times and over-pad
                    logging.info("Padding end of %s before wrapping for message %d.",
                            self.packet_lists[0], packet.index_sending)
                    self.packet_lists[0].append(None)
                self.padded_to_end = True
            logging.info("Wrap around to: %d", offset)
        else:
            packets = self.packet_lists[0]
        if offset < 0:
            # Duplicate packet.
            return None
        while offset >= len(packets):
            packets.append(None)
        packets[offset] = packet
        returned = []
        for i in range(2):
            packets = self.packet_lists[0]
            num_packets = len(packets)
            self.log("%d packets in buffer", num_packets)
            for j in range(num_packets):
                p = packets[0]
                if p is None:
                    break
                self.last_packet_index += 1
                packets.pop(0)
                returned.append(p)
                self.log("return %d, packets[0] size = %d, packets[1] size = %d", p.index_sending, len(self.packet_lists[0]), len(self.packet_lists[1]))
            if self.packet_lists[0] or not self.packet_lists[1]:
                break
            self.log("Flipping packet lists.")
            self.packet_lists = [self.packet_lists[1], []]
            self.padded_to_end = False
            packets = self.packet_lists[0]
            self.last_packet_index = 0
        return returned

    def PopIncomingError(self):
        """Errors in messages incoming to this device."""
        try:
            return self.incoming_error.get(block=False)
        except Empty:
            return None

    def PopIncomingAck(self):
        """Acks for messages incoming to this device."""
        try:
            return self.incoming_ok.get(block=False)
        except Empty:
            return None

    def PopOutgoingError(self):
        """Errors for messages sent from this device."""
        try:
            return self.outgoing_error.get(block=False)
        except Empty:
            return None

    def PopOutgoingAck(self):
        """Acks for messages sent from this device."""
        try:
            return self.outgoing_ok.get(block=False)
        except Empty:
            return None


    def ReadIntoBuffer(self):
        while True:
            byte = self.ser.read()
            if byte is None:
                return
            self.bytes.append(byte)


class Ack(object):
    def __init__(self, index, ok):
        """Data index and whether successful. index of 0 -> no ack"""
        if index is None:
            self.index = 0
        else:
            self.index = index
        self.ok = ok
        self.error = not ok

    def __str__(self):
        if self.ok:
            message = "ok"
        else:
            message = "error"
        return "%d %s" % (self.index, message)

SentMessage = namedtuple("SentMessage", ["index", "data"])
RETRY_PACKET = 42
NEW_PACKET = 43
NO_PACKET = 0

class Writer(Loggable):
    def __init__(self, ser, name=""):
        Loggable.__init__(self, name)
        self.ser = ser
        self.next_index = 1
        self.sent_messages = deque()
        self.retry_queue = deque()
        self.max_outgoing_length = 4
        self.last_send_time = time.time()
        self.all_quiet = True

    def done(self):
        if self.all_quiet:
            self.log("Done.")
        return self.all_quiet

    def Write(self, incoming_ack, outgoing_ack, tx_queue):
        """Writes the incoming_ack, and either new data or a retry if any."""
        if outgoing_ack.index != 0:
            self.all_quiet = False
            self.PopSentForAck(outgoing_ack)
        else:
            self.MaybeRetryAllSent()
        packet = Packet()
        packet.WithAck(incoming_ack.index, error=incoming_ack.error)
        packet_type = self.AddPacketData(tx_queue, packet)
        if packet.index_sending != 0 or packet.index_ack != 0:
            self.TransmitPacket(packet)
        self.all_quiet = len(self.retry_queue) == 0 and len(self.sent_messages) == 0
        return packet_type == NEW_PACKET

    def PopSentForAck(self, ack):
        """Updates the sent_messages list given an ack."""
        logging.info("Ack: %s", ack)
        found = False
        for ack_position, message in enumerate(self.sent_messages):
            if message.index == ack.index:
                found = True
                break
        if not found:
            # Likely an old or duplicate ack. Ignore it.
            self.log("Got an ack for an unknown message %d", ack.index)
            return
        if ack.error:
            ack_position += 1
        for i in range(ack_position):
            self.retry_queue.append(self.sent_messages.popleft())
        if ack.ok:
            self.sent_messages.popleft()

    def AddPacketData(self, tx_queue, packet):
        """Adds the data part of the packet."""
        packet_type = NO_PACKET
        if len(self.sent_messages) >= self.max_outgoing_length:
            #self.log("Too many sent; no write.")
            return packet_type
        if len(self.retry_queue) > 0:
            self.log("Retry rather than use queue.")
            sending = self.retry_queue.popleft()
            packet_type = RETRY_PACKET
        elif not tx_queue.empty():
            sending = SentMessage(self.next_index, tx_queue.get())
            self.next_index += 1
            self.next_index = self.next_index % 128
            # TODO: make sure tests catch this if missing
            if self.next_index == 0:
                self.next_index = 1
            packet_type = NEW_PACKET
        else:
            return NO_PACKET
        self.log("Sending packet: %d, %s", sending.index, sending.data)
        packet.WithData(sending.index, sending.data)
        self.sent_messages.append(sending)
        return packet_type

    def TransmitPacket(self, packet):
        """Sends the packet over the wire."""
        ints = packet.SerializeToInts()
        for b in ints:
            self.ser.write(b)
        self.last_send_time = time.time()

    def MaybeRetryAllSent(self):
        """Triggers retries if we get multiple no-index acks and have a
          non-empty sent_messages list.

         The risk here is that if we do it too soon, we'll spam the receiver.
        """
        if (len(self.sent_messages) > 0 and
                time.time() - self.last_send_time > 0.2):
            self.all_quiet = False
            self.retry_queue.append(self.sent_messages.popleft())
            self.log("Maybe retry all sent.")


class RXModule(object):
    def __init__(self, serial_connection):
        self.thread = RXThread(serial_connection, incoming_queue, outgoing_queue)
        self.thread.start()

    def Write(self, command):
        """Command is a string. Blocks until this command is sent."""
        self.thread.WriteMessage(command)

    def Read(self, timeout=0):
        """Returns a complete message, if incoming transmission is complete."""
        return self.thread.ReadMessage(timeout)

    def Clear(self):
        self.message = Message()

class RXThread(Thread, Loggable):
    def __init__(self, serial_connection, name=''):
        Thread.__init__(self)
        Loggable.__init__(self, name)
        self.ser = serial_connection
        self.incoming_message = None
        self.last_communication_time = time.time()
        self.rx_queue = Queue(5)
        self.tx_queue = Queue(5)
        self.daemon = True
        self.name = name
        self.reader = Reader(self.ser, name)
        self.writer = Writer(self.ser, name)
        self.messages_requested = 0
        self.messages_writer_accepted = 0

    def WriteMessage(self, command):
        self.tx_queue.put(command)
        self.messages_requested += 1

    def ReadMessage(self, timeout):
        try:
            self.log("Reading from queue of length: %d", self.rx_queue.qsize())
            return self.rx_queue.get(timeout=timeout).data
        except Empty:
            return None

    def run(self):
        while True:
            self.RxTxLoop()

    def done(self):
        return (self.messages_requested == self.messages_writer_accepted
                and self.writer.done()) or not self.is_alive()

    def RxTxLoop(self):
        """Called in a loop; handles a single rx/tx pair."""
        # Receive
        self.reader.Read(self.rx_queue)
        incoming_index_to_resend = self.reader.PopIncomingError()
        if incoming_index_to_resend:
            incoming_ack = Ack(index=incoming_index_to_resend, ok=False)
        else:
            incoming_index_to_ack = self.reader.PopIncomingAck()
            incoming_ack = Ack(index=incoming_index_to_ack, ok=True)
        send_error_index = self.reader.PopOutgoingError()
        if send_error_index:
            outgoing_ack = Ack(send_error_index, ok=False)
        else:
            send_ack_index = self.reader.PopOutgoingAck()
            outgoing_ack = Ack(send_ack_index, ok=True)
        if incoming_ack.index:
            self.log("Incoming ack %s", incoming_ack)
        if outgoing_ack.index:
            self.log("Outgoing ack %s", outgoing_ack)
        #self.log("Write from tx_queue of size %d", self.tx_queue.qsize())
        if self.writer.Write(
                incoming_ack=incoming_ack,
                outgoing_ack=outgoing_ack,
                tx_queue=self.tx_queue):
            self.messages_writer_accepted += 1
