import PySimpleGUI as sg
import json
import os.path
import threading
import time


import libeve.driver
from api import api, BotView


file_list_column = [
    [
        sg.Text("Bot: "),
        sg.Text("<No Bot Selected>", key="currently_selected_bot"),
    ],
    [
        sg.Button("Run", size=(5, 1), disabled=True, key="run"),
        sg.Button("Pause", size=(5, 1), disabled=True, key="pause"),
        sg.Button("Stop", size=(5, 1), disabled=True, key="stop"),
        sg.Button("Stop Safely", size=(10, 1), disabled=True, key="stop_safely"),
    ],
    [sg.Multiline(enable_events=True, size=(1000, 38), disabled=True, key="bot_log")],
    [
        sg.In(
            size=(25, 1),
            enable_events=True,
            readonly=True,
            visible=False,
            key="bot_config_file",
        ),
        sg.FileBrowse(),
    ],
]


layout = [
    [
        sg.Column(file_list_column),
    ]
]


class Application(object):
    def __init__(self):
        BotView.app = self
        self.window = sg.Window(
            "EVE Online - Bot Application", layout, size=(1024, 768)
        )
        self.bot_loaded = False
        self.bot_config_file = None
        self.bot_config = dict()
        self.bot_log = list()
        self.driver = None
        self.run_thread = None
        self.api_thread = threading.Thread(
            target=api.run, kwargs=dict(host="0.0.0.0", debug=False, use_reloader=False)
        )
        self.api_thread.daemon = True
        self.api_thread.start()
        self.pause_interrupt = threading.Event()
        self.stop_interrupt = threading.Event()
        self.stop_safely_interrupt = threading.Event()
        self.reset_run_thread()
        self.show()

    def reset_run_thread(self):
        del self.run_thread
        self.run_thread = threading.Thread(target=self.initiate_driver)
        self.run_thread.daemon = True

    def log(self, message):
        self.bot_log.append(message)
        self.window["bot_log"].update("\n".join(self.bot_log))

    def initiate_driver(self):
        try:
            self.log("starting bot...")
            self.driver = libeve.driver.BotDriver(
                self.bot_config_file,
                log_fn=self.log,
                pause_interrupt=self.pause_interrupt,
                pause_callback=self.pause_callback,
                stop_interrupt=self.stop_interrupt,
                stop_callback=self.stop_callback,
                stop_safely_interrupt=self.stop_safely_interrupt,
                stop_safely_callback=self.stop_safely_callback,
            )
            self.driver.bot.initialize()
            self.driver.start()
        except Exception as e:
            self.log(f"error: {e}")
        finally:
            self.window["run"].Update(disabled=False)
            self.window["pause"].update(disabled=True)
            self.window["stop"].update(disabled=True)
            self.window["stop_safely"].update(disabled=True)
            self.reset_run_thread()
            self.log("bot finished...")

    def bot_is_running(self):
        return self.run_thread.is_alive()

    def run(self):
        if not self.bot_is_running():
            self.run_thread.start()
            self.window["run"].Update(disabled=True)
            self.window["pause"].update(disabled=False)
            self.window["stop"].update(disabled=False)
            self.window["stop_safely"].update(disabled=False)
        else:
            self.log("bot is already running!")

    def pause(self):
        self.window["pause"].update(disabled=True)
        self.window["stop"].update(disabled=True)
        self.window["stop_safely"].update(disabled=True)
        if self.pause_interrupt.is_set():
            self.log("resuming execution...")
            self.pause_interrupt.clear()
            self.window["pause"].update(text="Pause")
        else:
            self.log("pausing execution...")
            self.pause_interrupt.set()
            self.window["pause"].update(text="Play")

    def pause_callback(self):
        self.window["pause"].update(disabled=False)
        if not self.driver.bot.paused:
            self.window["stop"].update(disabled=False)
            self.window["stop_safely"].update(disabled=False)
        else:
            self.log("paused!")

    def stop(self):
        self.window["pause"].update(disabled=True)
        self.window["stop"].update(disabled=True)
        self.window["stop_safely"].update(disabled=True)
        self.stop_interrupt.set()

    def stop_callback(self):
        self.window["run"].Update(disabled=False)
        self.window["pause"].update(disabled=True)
        self.window["stop"].update(disabled=True)
        self.window["stop_safely"].update(disabled=True)
        self.stop_interrupt.clear()
        self.reset_run_thread()
        self.log("stopped execution!")

    def stop_safely(self):
        self.window["pause"].update(disabled=True)
        self.window["stop"].update(disabled=True)
        self.window["stop_safely"].update(disabled=True)
        self.stop_safely_interrupt.set()

    def stop_safely_callback(self):
        self.window["run"].Update(disabled=False)
        self.window["pause"].update(disabled=True)
        self.window["stop"].update(disabled=True)
        self.window["stop_safely"].update(disabled=True)
        self.stop_safely_interrupt.clear()
        self.reset_run_thread()
        self.log("safely stopped execution!")

    def load(self, values):
        if not os.path.exists(values["bot_config_file"]):
            self.log(f'file does not exist: "{values["bot_config_file"]}"')
            return

        with open(values["bot_config_file"]) as f:
            self.bot_config = json.load(f)

        if not self.bot_config:
            self.log(f'invalid bot config: "{values["bot_config_file"]}"')
            self.bot_config = dict()
            return

        self.log(f'loaded bot file: "{values["bot_config_file"]}"')
        self.window["currently_selected_bot"].update(
            self.bot_config.get("uses", "Unknown")
        )
        self.window["run"].update(disabled=False)
        self.bot_config_file = values["bot_config_file"]
        self.bot_loaded = True

    def show(self):

        while True:

            event, values = self.window.read()

            try:
                if event == "Exit" or event == sg.WIN_CLOSED:
                    break
                elif event in ("run", "pause", "stop") and not self.bot_loaded:
                    self.log("no bot loaded")
                elif event == "run":
                    self.run()
                elif event == "pause":
                    self.pause()
                elif event == "stop":
                    self.stop()
                elif event == "stop_safely":
                    self.stop_safely()
                elif event == "bot_config_file":
                    self.load(values)
            except Exception as e:
                self.log(f'critical error: "{e}"')

        self.window.close()


if __name__ == "__main__":
    Application().run()
