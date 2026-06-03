import gdb # type: ignore


class PrintFlatList(gdb.Command):
    def __init__(self):
        super(PrintFlatList, self).__init__("print_flat_list", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        val = gdb.parse_and_eval(arg)
        size = int(val.type.template_argument(1))
        buffer = val["_buffer"]
        head = int(buffer[size]["_next"])
        tail = int(buffer[size]["_prev"])

        N = 4
        elems = []

        while head != size:
            node = buffer[head]

            elems.append(str(node["_value"]))
            head = node["_next"]

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


class PrintUint256(gdb.Command):
    def __init__(self):
        super(PrintUint256, self).__init__("print_uint256", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        def _split_int128(val):
            u64 = gdb.lookup_type("uint64_t")
            ptr = val.address.cast(u64.pointer())
            lo = int(ptr[0])
            hi = int(ptr[1])
            return lo, hi
        
        val = gdb.parse_and_eval(arg)
        val = val.cast(val.type.strip_typedefs())

        lo128 = val["data"][0]
        hi128 = val["data"][1]

        lo_lo, lo_hi = _split_int128(lo128)
        hi_lo, hi_hi = _split_int128(hi128)
        words = [lo_lo, lo_hi, hi_lo, hi_hi]

        bits = []

        for word_index in range(3, -1, -1):
            w = words[word_index]
            
            if w == 0:
                continue

            base = word_index * 64

            for bit in range(63, -1, -1):
                if (w >> bit) & 1:
                    bits.append(base + bit)

        print("uint256_t (msb→lsb):", bits)

PrintUint256()


class PrintQueue(gdb.Command):
    def __init__(self):
        super(PrintQueue, self).__init__("pq", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        val = gdb.parse_and_eval(arg)
        type = str(val.type.strip_typedefs())

        if type.startswith("FlatList<") or type.startswith("FlatQueue<") or type.startswith("FlatQueue_OA<"):
            PrintFlatList().invoke(arg, from_tty)
        elif type.startswith("RingBuffer<"):
            PrintRingBuffer().invoke(arg, from_tty)
        elif type.startswith("RingBufferSPSC<"):
            PrintRingBufferSPSC().invoke(arg, from_tty)
        elif type.startswith("RingBufferSPMC<"):
            PrintRingBufferSPMC().invoke(arg, from_tty)
        elif type.startswith("uint256_t"):
            PrintUint256().invoke(arg, from_tty)
        else:
            print(f"Unsupported queue type: {type}")
        

PrintQueue()
