import json
import os
import time
import threading
import traceback
from twilio.rest import Client

from .bots.autopilot import AutoPilotBot
from .bots.mining import MiningBot


account_sid = os.environ.get("TWILIO_ACCOUNT_SID")
auth_token = os.environ.get("TWILIO_AUTH_TOKEN")
twilio_configured = account_sid and auth_token

client = None
if twilio_configured:
    client = Client(account_sid, auth_token)


class BotDriver(object):

    registered_bots = {"AutoPilotBot": AutoPilotBot, "MiningBot": MiningBot}

    def __init__(
        self,
        driver_filename,
        log_fn=print,
        pause_interrupt: threading.Event = None,
        pause_callback=None,
        stop_interrupt: threading.Event = None,
        stop_callback=None,
        stop_safely_interrupt: threading.Event = None,
        stop_safely_callback=None,
    ):
        self.driver_filename = driver_filename
        with open(self.driver_filename) as driver_file:
            self.driver = json.load(driver_file)
        self.muted = not self.driver.get("with_narration", False)
        self.bot_name = self.driver.get("uses")
        self.start_from = self.driver.get("start_from")
        self.focus_enabled = self.driver.get("focus", False)
        self.loop = self.driver.get("loop", False)
        self.sms_number = self.driver.get("sms_number")
        self.scanners = self.driver.get("scanners", [])
        self.args = self.driver.get("args", {})
        self.started = False
        self.log_fn = log_fn
        self.pause_interrupt = pause_interrupt
        self.pause_callback = pause_callback
        self.stop_interrupt = stop_interrupt
        self.stop_callback = stop_callback
        self.stop_safely_interrupt = stop_safely_interrupt
        self.stop_safely_callback = stop_safely_callback
        if not self.bot_name:
            raise Exception(f"`uses` key must be present in {self.driver_filename}")
        if self.bot_name not in BotDriver.registered_bots:
            raise Exception(f"`{self.bot_name}` is not a registered bot")
        self.bot = BotDriver.registered_bots[self.bot_name](
            log_fn=self.log_fn,
            pause_interrupt=self.pause_interrupt,
            pause_callback=self.pause_callback,
            stop_interrupt=self.stop_interrupt,
            stop_callback=self.stop_callback,
            stop_safely_interrupt=self.stop_safely_interrupt,
            stop_safely_callback=self.stop_safely_callback,
            **self.args,
        )

        self.start_scanners()

    def start_scanners(self):
        # for scanner in self.scanners:
        #    with ThreadPoolExecutor(max_workers=1) as executor:
        #        future = executor.submit(registered_scanners[scanner])
        pass

    def start(self):
        if not self.bot.tree:
            return self.say("bot is not initialized!", narrate=False)
        try:
            while True:
                for step in self.driver.get("steps", list()):
                    if not self.started and self.start_from and self.start_from != step:
                        continue
                    if self.focus_enabled:
                        self.bot.focus()
                    self.started = True
                    fn = getattr(self.bot, step)
                    if not (fn and callable(fn)):
                        raise Exception(
                            f"`{step}` is not a registered action in bot `{self.bot_name}`"
                        )
                    self.log_fn(f"== running step: {step}")
                    fn()
                if not self.loop:
                    break
        except Exception as e:
            traceback.print_exc()
            if twilio_configured and self.sms_number:
                message = client.messages.create(
                    body="Mining Bot failed.",
                    from_="+13855955197",
                    to=self.sms_number,
                )
            while True:
                self.driver.bot.say("Error, attention needed")
                time.sleep(5)
        finally:
            self.bot.tree.cleanup()
