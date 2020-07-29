from random import Random
from threading import Thread, Semaphore
from queue import Queue
from pathlib import Path


class FaultableFileRelay(Thread):
    """Used for tests. Copies data from one file to the next, streaming. Optionally adds errors."""
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

