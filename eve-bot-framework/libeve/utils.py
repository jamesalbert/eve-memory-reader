import ctypes
import win32gui


def get_screensize():
    return ctypes.windll.user32.GetSystemMetrics(
        0
    ), ctypes.windll.user32.GetSystemMetrics(1)


def with_node(query={}, address=None, select_many=False, contains=False):
    def inner(func):
        def wrapper(self):
            node = self.tree.find_node(query, address, select_many, contains)
            return func(self, node)

        return wrapper

    return inner


def window_enumeration_handler(hwnd, top_windows):
    top_windows.append((hwnd, win32gui.GetWindowText(hwnd)))
