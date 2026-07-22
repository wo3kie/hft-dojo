import gdb # type: ignore


class PrintFlatList(gdb.Command):
    def __init__(self):
        super(PrintFlatList, self).__init__("print_flat_list", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        N = 4
        elems = []
        val = gdb.parse_and_eval(arg)
        buffer = val["_pool"]["_buffer"]["_buffer"]
        slot = int(val["_head"])

        while slot != -1:
            node = buffer[slot]

            elems.append(str(node["_value"]))
            slot = node["_next"]

        if (N == -1) or (len(elems) <= 2 * N):
            shown = elems
        else:
            shown = elems[:N] + ["..."] + elems[-N:]

        print(f"{val.type.strip_typedefs()} [ {", ".join(shown)} ]")

PrintFlatList()


class PrintRingBuffer(gdb.Command):
    def __init__(self):
        super(PrintRingBuffer, self).__init__("print_ring_buffer", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        N = 4
        elems = []
        val = gdb.parse_and_eval(arg)
        size = val.type.template_argument(1)
        buffer = val["_buffer"]["_buffer"]
        head = int(val["_head"])
        tail = int(val["_tail"])

        while head != tail:
            elems.append(str(buffer[head]))
            head = (head + 1) % (size + 1)

        if (N == -1) or (len(elems) <= 2 * N):
            shown = elems
        else:
            shown = elems[:N] + ["..."] + elems[-N:]

        print(f"{val.type.strip_typedefs()} [ {", ".join(shown)} ]")

PrintRingBuffer()


class PrintRingBufferSPSC(gdb.Command):
    def __init__(self):
        super(PrintRingBufferSPSC, self).__init__("print_ring_buffer_spsc", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        N = 4
        elems = []
        val = gdb.parse_and_eval(arg)
        size = val.type.template_argument(1)
        buffer = val["_buffer"]["_buffer"]
        head = int(val["_head"]["_M_i"])
        tail = int(val["_tail"]["_M_i"])

        while head != tail:
            elems.append(str(buffer[head]))
            head = (head + 1) % (size + 1)

        if (N == -1) or (len(elems) <= 2 * N):
            shown = elems
        else:
            shown = elems[:N] + ["..."] + elems[-N:]

        print(f"{val.type.strip_typedefs()} [ {", ".join(shown)} ]")

PrintRingBufferSPSC()

class PrintPriceBits(gdb.Command):
    def __init__(self):
        super(PrintPriceBits, self).__init__("print_price_bits", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        val = gdb.parse_and_eval(arg)
        val = val.cast(val.type.strip_typedefs())

        N = 4
        elems = []
        data_field = val["data"]
        size = val.type.template_argument(0)
        chunks = data_field.type.sizeof // gdb.lookup_type("uint64_t").sizeof
        words = [int(data_field[i]) for i in range(chunks)]

        for word_index in range(chunks - 1, -1, -1):
            w = words[word_index]
            if w == 0:
                continue

            base = word_index * 64

            for bit in range(63, -1, -1):
                if (w >> bit) & 1:
                    elems.append(base + bit)

        if (N == -1) or (len(elems) <= 2 * N):
            shown = elems
        else:
            shown = elems[:N] + ["..."] + elems[-N:]

        print(f"{val.type.strip_typedefs()} (msb→lsb):", [ {", ".join(shown)} ])

PrintPriceBits()


class PrintQueue(gdb.Command):
    def __init__(self):
        super(PrintQueue, self).__init__("pq", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        val = gdb.parse_and_eval(arg)
        type = str(val.type.strip_typedefs())

        if type.startswith("FlatList<") or type.startswith("FlatQueue<") or type.startswith("FlatQueue<"):
            PrintFlatList().invoke(arg, from_tty)
        elif type.startswith("RingBuffer<"):
            PrintRingBuffer().invoke(arg, from_tty)
        elif type.startswith("RingBufferSPSC<"):
            PrintRingBufferSPSC().invoke(arg, from_tty)
        elif type.startswith("PriceBits"):
            PrintPriceBits().invoke(arg, from_tty)
        else:
            print(f"Unsupported queue type: {type}")
       

PrintQueue()
