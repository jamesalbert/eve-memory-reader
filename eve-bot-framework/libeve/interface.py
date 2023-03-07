from libeve.utils import get_screensize

import ctypes
import json
import os


if os.environ.get("ENV", "production") == "production":
    eve_memory_reader = ctypes.WinDLL("./eve-memory-reader.dll")
else:
    eve_memory_reader = ctypes.WinDLL("../x64/Release/eve-memory-reader.dll")

eve_memory_reader.initialize.argtypes = []
eve_memory_reader.initialize.restype = ctypes.c_int
eve_memory_reader.read_ui_trees.argtypes = []
eve_memory_reader.read_ui_trees.restype = None
eve_memory_reader.get_ui_json.argtypes = []
eve_memory_reader.get_ui_json.restype = ctypes.c_char_p
eve_memory_reader.free_ui_json.argtypes = []
eve_memory_reader.free_ui_json.restype = None
eve_memory_reader.cleanup.argtypes = []
eve_memory_reader.cleanup.restype = None


class UITreeNode(object):
    def __init__(self, **node):
        self.address: int = node.get("address")
        self.type: str = node.get("type")
        self.attrs = node.get("attrs", dict())
        self.x: int = node.get("x", 0)
        self.y: int = node.get("y", 0)
        self.parent: int = node.get("parent")
        self.data = dict()  # arbitrary data
        self.children: list[int] = list()
        for child in node.get("children", list()):
            self.children.append(child.get("address"))


class UITree(object):
    def __init__(self):
        self.nodes: dict[int, UITreeNode] = dict()
        self.width_ratio = 0
        self.height_ratio = 0
        ret = eve_memory_reader.initialize()
        if ret != 0:
            raise Exception(f"Failed to initialize: {ret}")
        self.refresh()

    def cleanup(self):
        eve_memory_reader.cleanup()

    def ingest(self, tree, x=0, y=0, parent=None):
        node = UITreeNode(**{**tree, **dict(x=x, y=y, parent=parent)})
        self.nodes[node.address] = node
        for child in tree.get("children", list()):
            real_x = x + (child["attrs"].get("_displayX", 0) or 0)
            real_y = y + (child["attrs"].get("_displayY", 0) or 0)
            self.ingest(child, real_x, real_y, tree["address"])

    def load(self, tree):
        self.nodes = dict()
        self.ingest(tree)
        try:
            if (
                tree["attrs"].get("_displayWidth", 0) == None
                or tree["attrs"].get("_displayHeight", 0) == None
            ):
                raise ZeroDivisionError
            screensize = get_screensize()
            self.width_ratio = screensize[0] / tree["attrs"].get("_displayWidth", 0)
            self.height_ratio = screensize[1] / tree["attrs"].get("_displayHeight", 0)
        except ZeroDivisionError:
            self.refresh()

    def refresh(self):
        eve_memory_reader.read_ui_trees()
        tree_bytes = eve_memory_reader.get_ui_json()
        eve_memory_reader.free_ui_json()
        if not tree_bytes:
            print("no ui trees found")
            return
        try:
            tree_str = tree_bytes.decode("utf-8", errors="ignore")
            with open("super.debug.json", "w") as super_debug_json:
                super_debug_json.write(tree_str)
            tree = json.loads(tree_str)
            self.load(tree)
            with open("debug.json", "w") as debug_file:
                debug_file.write(json.dumps(tree, indent=2))
        except UnicodeDecodeError as e:
            print(f"error reading ui trees: {e}")
            return
        except ValueError as e:
            print(f"error reading ui trees: {e}")
            return

    def find_node(
        self, query={}, address=None, type=None, select_many=False, contains=False
    ):
        self.refresh()
        nodes = list()

        for _, node in self.nodes.items():
            if address and node.address != address:
                continue
            if type and node.type != type:
                continue
            if all(
                [
                    node.attrs.get(qkey) == qval
                    if not contains
                    else qval in str(node.attrs.get(qkey, ""))
                    for qkey, qval in query.items()
                ]
            ):
                nodes.append(node)
                if not select_many:
                    break

        if nodes and not select_many:
            return nodes[0]
        return nodes
