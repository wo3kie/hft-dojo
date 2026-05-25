import gdb # type: ignore

class PrintFlatQueue(gdb.Command):
    def __init__(self):
        super(PrintFlatQueue, self).__init__("print_flat_queue", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        val = gdb.parse_and_eval(arg)
        head = int(val["_head"])
        buffer = val["_buffer"]

        N = 4
        elems = []

        while head != -1:
            storage_head = buffer[head]
            elems.append(str(storage_head["value"]))
            head = int(storage_head["next"])

        if (N == -1) or (len(elems) <= 2 * N):
            shown = elems
        else:
            shown = elems[:N] + ["..."] + elems[-N:]

        type = val.type.template_argument(0)
        size = int(val.type.template_argument(1))

        print(f"FlatQueue<{type}, {size}> [ {", ".join(shown)} ]")

PrintFlatQueue()

class PrintRingBuffer(gdb.Command):
    def __init__(self):
        super(PrintRingBuffer, self).__init__("print_ring_buffer", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        val = gdb.parse_and_eval(arg)
        size = val.type.template_argument(1)
        buffer = val["_buffer"]
        head = int(val["_head"])
        tail = int(val["_tail"])

        N = 4
        elems = []

        while head != tail:
            elems.append(str(buffer[head]))
            head = (head + 1) % (size + 1)

        if (N == -1) or (len(elems) <= 2 * N):
            shown = elems
        else:
            shown = elems[:N] + ["..."] + elems[-N:]

        print(f"RingBuffer<{val.type.template_argument(0)}, {size}> [ {", ".join(shown)} ]")

PrintRingBuffer()

class PrintRingBufferSPSC(gdb.Command):
    def __init__(self):
        super(PrintRingBufferSPSC, self).__init__("print_ring_buffer_spsc", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        val = gdb.parse_and_eval(arg)
        size = val.type.template_argument(1)
        buffer = val["_buffer"]
        head = int(val["_head"]["_M_i"])
        tail = int(val["_tail"]["_M_i"])

        N = 4
        elems = []

        while head != tail:
            elems.append(str(buffer[head]))
            head = (head + 1) % (size + 1)

        if (N == -1) or (len(elems) <= 2 * N):
            shown = elems
        else:
            shown = elems[:N] + ["..."] + elems[-N:]

        print(f"RingBufferSPSC<{val.type.template_argument(0)}, {size}> [ {", ".join(shown)} ]")

PrintRingBufferSPSC()

class PrintRingBufferSPMC(gdb.Command):
    def __init__(self):
        super(PrintRingBufferSPMC, self).__init__("print_ring_buffer_spmc", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        val = gdb.parse_and_eval(arg)
        size = val.type.template_argument(1)
        buffer = val["_buffer"]
        popped = int(val["_popped"]["_M_i"])
        pushed = int(val["_pushed"]["_M_i"])

        N = 4
        elems = []

        while popped != pushed:
            elems.append(str(buffer[popped % size]))
            popped += 1

        if (N == -1) or (len(elems) <= 2 * N):
            shown = elems
        else:
            shown = elems[:N] + ["..."] + elems[-N:]

        print(f"RingBufferSPMC<{val.type.template_argument(0)}, {size}> [ {", ".join(shown)} ]")

PrintRingBufferSPMC()