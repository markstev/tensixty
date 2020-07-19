import serial
import queue
import logging
import time
from random import Random

class SerialConnection(object):
    def __init__(self, port, baud):
        pass

    def CreateWithHardware(port, baud):
        return HardwareSerialConnection(port, baud)

    def CreateWithFile(port, baud):
        return FileSerialConnection(port, baud)

    def write(self, byte):
        pass

    def read(self):
        """Returns empty char if there is no data to read."""
        pass

class HardwareSerialConnection(SerialConnection):
    def __init__(self, port, baud):
        self.ser = serial.Serial(port, baud, timeout=0, rtscts=True)
        # This sleep is critical -- the serial line needs quiet time to initialize.
        logging.info("Ser stuff %s %s %s %s %s %s %s",
                self.ser.inWaiting(),
                self.ser.getCD(),
                self.ser.getCTS(),
                self.ser.getDSR(),
                self.ser.getRI(),
                self.ser.readable(),
                self.ser.writable()
                )
        time.sleep(2.0)
        logging.info("Ser stuff %s %s %s %s %s %s %s",
                self.ser.inWaiting(),
                self.ser.getCD(),
                self.ser.getCTS(),
                self.ser.getDSR(),
                self.ser.getRI(),
                self.ser.readable(),
                self.ser.writable()
                )

    def write(self, byte):
        b = bytes([byte])
        self.ser.write(b)

    def read(self):
        b = self.ser.read()
        if b == b'':
            return None
        else:
            result = int.from_bytes(b, byteorder='big')
            return result

class FileSerialConnection(SerialConnection):
    def __init__(self, port, baud, incoming_filename=None, outgoing_filename=None):
        if incoming_filename:
            self.incoming_filename = incoming_filename
        else:
            self.incoming_filename = '/tmp/arduinoio_hw_abstraction_a_%s' % port
        if outgoing_filename:
            self.outgoing_filename = outgoing_filename
        else:
            self.outgoing_filename = '/tmp/arduinoio_hw_abstraction_b_%s' % port
        with open(self.incoming_filename, 'w') as fcreate:
            pass
        self.incoming = open(self.incoming_filename, 'rb')
        self.outgoing = open(self.outgoing_filename, 'wb')
        logging.info("Filename = %s", self.incoming_filename)
        logging.info("Filename = %s", self.outgoing_filename)
        self.port = port
        self.baud = baud

    def write(self, byte):
        self.outgoing.write(bytes([byte]))
        self.outgoing.flush()
        logging.info('write: %s', byte)

    def read(self):
        result = self.incoming.read(1)
        if result == b'':
            # Avoids EOF getting set and skipping future reads.
            self.incoming.seek(self.incoming.tell())
            return None
        result = int.from_bytes(result, byteorder='big')
        logging.info("Read %d", result)
        return result

    def MakePair(self):
        return FileSerialConnection(self.port, self.baud, self.outgoing_filename, self.incoming_filename)

    def __del__(self):
        self.incoming.close()
        self.outgoing.close()

class QueueSerialConnection(SerialConnection):
    def __init__(self, port, baud):
        self.port = port
        self.baud = baud
        self.incoming = queue.Queue(100)
        self.outgoing = queue.Queue(100)

    def write(self, byte):
        self.outgoing.put(byte)

    def read(self):
        qsize = self.incoming.qsize()
        #   if qsize > 0:
        #       logging.info("Queue length = %d", qsize)
        try:
            return self.incoming.get(block=False)
        except queue.Empty:
            return None

    def MakePair(self):
        new_queue = QueueSerialConnection(self.port, self.baud)
        new_queue.incoming = self.outgoing
        new_queue.outgoing = self.incoming
        return new_queue


class FaultableSerialConnection(SerialConnection):
    def __init__(self, base_connection, error_rate, seed=42, mutation_rate=1.0):
        self.base_connection = base_connection
        self.error_rate = error_rate
        self.random = Random(seed)
        self.mutation_rate = mutation_rate

    def read(self):
        return self.base_connection.read()
        #f not self.ShouldError():
        #   return self.base_connection.read()
        #f self.random.random() < self.mutation_rate:
        #   # Replace what's in there already
        #   byte = self.base_connection.read()
        #   rando = self.RandomByte()
        #   logging.info("Replaced %s with %s", byte, rando)
        #   return rando
        #lif self.random.random() < 0.5:
        #   # Data add
        #   return self.RandomByte()
        #lse:
        #   # Data drop
        #   self.base_connection.read()
        #   return b''


    def write(self, byte):
        if not self.ShouldError():
            return self.base_connection.write(byte)
        if self.random.random() < self.mutation_rate:
            rando = self.RandomByte()
            logging.info("Replaced %s with %s", byte, rando)
            self.base_connection.write(rando)
        else:
            # Drop/add a byte
            if self.random.random() < 0.5:
                self.base_connection.write(self.RandomByte())
                self.base_connection.write(byte)

    def ShouldError(self):
        return self.random.random() < self.error_rate

    def RandomByte(self):
        return self.random.randint(0, 255)

    def MakePair(self):
        return FaultableSerialConnection(
                self.base_connection.MakePair(),
                self.error_rate,
                self.mutation_rate,
                42)


