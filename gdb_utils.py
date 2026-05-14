import gdb # type: ignore

class PrintFlatList(gdb.Command):
    def __init__(self):
        super(PrintFlatList, self).__init__("print_flat_list", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        val = gdb.parse_and_eval(arg)
        head = int(val["_head"])
        size = val.type.template_argument(0)
        storage = val["_storage"]["_M_elems"]

        N = 4
        elems = []

        while head != -1:
            node = storage[head]
            elems.append(str(node["value"]))
            head = int(node["next"])

        if (N == -1) or (len(elems) <= 2 * N):
            shown = elems
        else:
            shown = elems[:N] + ["..."] + elems[-N:]

        print(f"FlatList<{size}, {val.type.template_argument(1)}> [ {", ".join(shown)} ]")

PrintFlatList()
