from libeve.utils import get_screensize, window_enumeration_handler
from libeve.interface import UITree


import sys
import time
import threading
import win32api, win32con, win32gui
import win32com.client


speaker = win32com.client.Dispatch("SAPI.SpVoice")


class Bot(object):
    def __init__(
        self,
        log_fn=print,
        pause_interrupt: threading.Event = None,
        pause_callback=None,
        stop_interrupt: threading.Event = None,
        stop_callback=None,
        stop_safely_interrupt: threading.Event = None,
        stop_safely_callback=None,
    ):
        self.log_fn = log_fn
        self.pause_interrupt = pause_interrupt
        self.pause_callback = pause_callback
        self.stop_interrupt = stop_interrupt
        self.stop_callback = stop_callback
        self.stop_safely_interrupt = stop_safely_interrupt
        self.stop_safely_callback = stop_safely_callback
        self.stopping_safely = False
        self.paused = False
        self.tree = None

    def initialize(self):
        self.say("Initializing")
        self.say(f"detected screen size: {get_screensize()}", narrate=False)
        self.tree = UITree()
        self.say("Ready")

    def check_pause_interrupt(self):
        while self.pause_interrupt and self.pause_interrupt.is_set():
            if not self.paused:
                self.paused = True
                if self.pause_callback and callable(self.pause_callback):
                    self.pause_callback()
            time.sleep(0.25)
        if self.paused:
            self.paused = False
            if self.pause_callback and callable(self.pause_callback):
                self.pause_callback()

    def stop(self):
        self.tree = None

    def check_stop_interrupt(self):
        if self.stop_interrupt and self.stop_interrupt.is_set():
            if self.stop_callback and callable(self.stop_callback):
                self.stop()
                self.stop_callback()
                sys.exit(1)

    def check_stop_safely_interrupt(self):
        if self.stop_safely_interrupt and self.stop_safely_interrupt.is_set():
            if (
                not self.stopping_safely
                and self.stop_safely_callback
                and callable(self.stop_safely_callback)
            ):
                self.log_fn("stop safely interrupt triggered")
                self.stopping_safely = True
                self.ensure_within_station()
                self.stop_safely_callback()
                sys.exit(1)

    def check_interrupts(self):
        self.check_pause_interrupt()
        self.check_stop_interrupt()
        self.check_stop_safely_interrupt()

    def speak(self, text):
        threading.Thread(target=speaker.Speak, args=(text,)).start()

    def say(self, text, narrate=True):
        self.check_interrupts()
        self.log_fn(text)
        if narrate:
            self.speak(text)

    def focus(self, prefix="eve - "):
        self.check_interrupts()
        top_windows = []
        win32gui.EnumWindows(window_enumeration_handler, top_windows)
        for i in top_windows:
            if prefix in i[1].lower():
                win32gui.ShowWindow(i[0], 5)
                win32gui.SetForegroundWindow(i[0])
                break
        time.sleep(1)

    def move_cursor_to_node(self, node):
        self.check_interrupts()
        x = (
            int(node.x * self.tree.width_ratio)
            + node.attrs.get("_displayWidth", 0) // 2
        )
        y = (
            int(node.y * self.tree.height_ratio)
            + node.attrs.get("_displayHeight", 0) // 2
        )
        print(f"setting cursor to {x}, {y}")
        win32api.SetCursorPos((x, y))
        time.sleep(1)
        return x, y

    def click_node(self, node, right_click=False, times=1, expect=[], expect_args={}):
        self.check_interrupts()
        x, y = self.move_cursor_to_node(node)
        down_event = (
            win32con.MOUSEEVENTF_RIGHTDOWN
            if right_click
            else win32con.MOUSEEVENTF_LEFTDOWN
        )
        up_event = (
            win32con.MOUSEEVENTF_RIGHTUP if right_click else win32con.MOUSEEVENTF_LEFTUP
        )
        for i in range(times):
            win32api.mouse_event(down_event, x, y, 0, 0)
            time.sleep(1)
            win32api.mouse_event(up_event, x, y, 0, 0)
            time.sleep(1)
            print("clicked")
        for expectation in expect:
            if not self.wait_for(expectation, until=10, **expect_args):
                win32api.mouse_event(down_event, x, y, 0, 0)
                time.sleep(1)
                win32api.mouse_event(up_event, x, y, 0, 0)
                time.sleep(1)
                while not (tmp_node := self.tree.find_node(address=node.address)):
                    time.sleep(2)
                node = tmp_node
                return self.click_node(
                    node=node,
                    right_click=right_click,
                    times=times,
                    expect=expect,
                    expect_args=expect_args,
                )
        time.sleep(1)
        return x, y

    def drag_node_to_node(self, src_node, dest_node):
        self.check_interrupts()
        x, y = self.move_cursor_to_node(src_node)
        win32api.mouse_event(win32con.MOUSEEVENTF_LEFTDOWN, x, y, 0, 0)
        time.sleep(1)
        x, y = self.move_cursor_to_node(dest_node)
        win32api.mouse_event(win32con.MOUSEEVENTF_LEFTUP, x, y, 0, 0)
        time.sleep(1)
        print("dragged")
        return x, y

    def wait_for(
        self,
        query={},
        address=None,
        type=None,
        select_many=False,
        contains=False,
        until=0,
    ):
        self.check_interrupts()
        print(
            f"waiting for query={query}, address={address}, type={type}, select_many={select_many}, contains={contains}"
        )
        started = time.time()
        while not (
            node := self.tree.find_node(query, address, type, select_many, contains)
        ):
            if until and time.time() - started >= until:
                break
        time.sleep(1)
        return node

    def undock(self):
        undock_btn = self.wait_for({"_setText": "Undock"}, type="LabelThemeColored")
        self.say("undocking")
        self.click_node(undock_btn)
        self.wait_for_overview()

    def wait_for_overview(self):
        self.wait_for({"_setText": "Overview"}, type="EveLabelSmall", contains=True)

    def wait_until_warp_finished(self):
        self.wait_for({"_setText": "Warp Drive Active"})
        self.say("warp drive active")
        while self.tree.find_node({"_setText": "Warp Drive Active"}):
            time.sleep(2)
        self.say("warp drive disengaged")
        time.sleep(5)

    def wait_until_jump_finished(self):
        self.wait_for({"_setText": "Jumping"})
        self.say("jumping")
        while self.tree.find_node({"_setText": "Jumping"}):
            time.sleep(2)
        time.sleep(2)

    def recall_drones(self):

        if not self.wait_for(
            {"_setText": "Drones in Bay (0)"}, type="EveLabelMedium", until=5
        ):
            return

        self.say("recalling drones")

        drones_in_space = self.wait_for(
            {"_setText": "Drones in Local Space ("},
            type="EveLabelMedium",
            contains=True,
        )

        self.click_node(drones_in_space, right_click=True)

        return_btn = self.wait_for(
            {"_setText": "Return to Drone Bay ("}, type="EveLabelMedium", contains=True
        )

        self.click_node(return_btn)

        time.sleep(2)

        if not self.wait_for(
            {"_setText": "Drones in Local Space (0)"}, type="EveLabelMedium", until=5
        ):
            self.recall_drones()

    def ensure_within_station(self):

        undock_btn = self.wait_for(
            {"_setText": "Undock"}, type="LabelThemeColored", until=5
        )
        if undock_btn:
            return

        self.recall_drones()

        while True:
            self.wait_for_overview()
            self.say("Finding station")
            time.sleep(3)

            station = self.wait_for(
                {"_text": self.station}, type="OverviewLabel", until=10
            )

            if not station:

                warpto_tab = self.wait_for(
                    {"_setText": "WarpTo"}, type="LabelThemeColored"
                )

                self.click_node(
                    warpto_tab,
                    times=2,
                    expect=[{"_text": self.station}],
                    expect_args={"type": "OverviewLabel"},
                )

                station = self.wait_for({"_text": self.station}, type="OverviewLabel")

            self.click_node(station)

            dock_btn = self.wait_for({"_name": "selectedItemDock"}, type="Container")

            self.click_node(dock_btn)

            if self.wait_for({"_setText": "Establishing Warp Vector"}, until=5):
                self.wait_until_warp_finished()

            break

        self.wait_for({"_setText": "Undock"}, type="LabelThemeColored")
        time.sleep(5)

    def fleetup(self):
        if not self.wait_for({"_name": "fleetwindow"}, type="FleetWindow", until=5):
            self.wait_for({"_setText": "Join Fleet?"}, type="EveCaptionLarge")
            yes_btn = self.wait_for({"_setText": "Yes"}, type="LabelThemeColored")
            self.click_node(yes_btn)
        self.say("fleet ready")
