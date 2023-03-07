import time

from libeve.bots import Bot


class AutoPilotBot(Bot):
    def __init__(
        self,
        log_fn=print,
        pause_interrupt=None,
        pause_callback=None,
        stop_interrupt=None,
        stop_callback=None,
        stop_safely_interrupt=None,
        stop_safely_callback=None,
    ):
        super().__init__(
            log_fn=log_fn,
            pause_interrupt=pause_interrupt,
            pause_callback=pause_callback,
            stop_interrupt=stop_interrupt,
            stop_callback=stop_callback,
            stop_safely_interrupt=stop_safely_interrupt,
            stop_safely_callback=stop_safely_callback,
        )

    def go(self):
        while True:

            route = self.wait_for(
                {"_name": "markersParent"}, type="Container", until=15
            )

            if not route:
                break

            for waypoint_id in route.children:

                def jump():
                    jump_btn = self.wait_for(
                        {"_setText": "Jump through stargate"},
                        type="EveLabelMedium",
                        until=5,
                    )

                    if jump_btn:
                        self.say("jumping")
                        self.click_node(jump_btn)
                        self.wait_for({"_setText": "Establishing Warp Vector"}, until=5)
                        self.wait_until_warp_finished()
                        time.sleep(10)

                def dock():
                    dock_btn = self.wait_for(
                        {"_setText": "Dock"}, type="EveLabelMedium", until=5
                    )

                    if not dock_btn:
                        return -1

                    self.say("docking")
                    self.click_node(dock_btn)
                    self.wait_for({"_setText": "Establishing Warp Vector"}, until=5)
                    self.wait_until_warp_finished()
                    self.wait_for_overview()

                def jump_or_dock():
                    waypoint = self.tree.nodes[waypoint_id]
                    self.click_node(waypoint, right_click=True)

                    if len(route.children) == 1:
                        if dock() == -1:
                            jump()
                        return
                    else:
                        jump()
                        return

                jump_or_dock()
                break
