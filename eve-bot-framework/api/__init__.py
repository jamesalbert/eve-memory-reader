from flask import Flask
from flask_classful import FlaskView

api = Flask(__name__)


class BotView(FlaskView):

    app = None

    def healthcheck(self):
        return "200"

    def pause(self):
        BotView.app.pause()
        return {"status": "ok", "paused": not BotView.app.driver.bot.paused}

    def stop(self):
        BotView.app.stop()
        return {"status": "ok"}

    def stop_safely(self):
        BotView.app.stop_safely()
        return {"status": "ok"}


BotView.register(api, route_base="/api/v1/")
