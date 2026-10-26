#!/usr/bin/env python3
"""Generate headless simulator replays for the Math Games flows.

Coordinates follow components/app_game_runtime/private/ui_layout.h (480x480).
Run: python tests/make_replays.py, then
     python mosaico.py game sim --project projects/math_games --headless \
         --replay projects/math_games/tests/replays/<name>.json --frames <frames>
"""
import json
from pathlib import Path

OUT = Path(__file__).resolve().parent / "replays"
STEP = 3          # Frames between taps.
AI_WAIT = 20      # Frames for a computer move (15-tick delay plus margin).

HOME_CARD = [(240, 178), (240, 290), (240, 402)]
BACK, RIGHT = (60, 28), (422, 28)
MODE = [(126, 129), (354, 129)]
SIDE = [(126, 239), (354, 239)]
LEVEL = [(88, 336), (240, 336), (392, 336)]
START = (240, 425)
RESULT_MENU, RESULT_AGAIN = (131, 314), (321, 314)
COMPASS = ["NW", "N", "NE", "W", None, "E", "SW", "S", "SE"]


def oc_cell(i):
    return (12 + (i % 6) * 64 + 32, 70 + (i // 6) * 64 + 32)


OC_MARK = {"X": (438, 202), "O": (438, 318)}


def ut_board(b):
    return (69 + (b % 3) * 118 + 56, 112 + (b // 3) * 118 + 56)


def ut_cell(c):
    return (18 + (c % 3) * 114 + 57, 116 + (c // 3) * 114 + 57)


def da_cell(i):
    return (12 + (i % 5) * 62 + 31, 112 + (i // 5) * 62 + 31)


def da_dir(name):
    slot = COMPASS.index(name)
    return (330 + (slot % 3) * 50 + 23, 160 + (slot // 3) * 50 + 23)


class Replay:
    def __init__(self):
        self.frame = 2
        self.events = []

    def tap(self, point, wait=STEP):
        self.events.append({"frame": self.frame, "type": "tap", "x": point[0], "y": point[1]})
        self.frame += wait
        return self

    def wait(self, frames):
        self.frame += frames
        return self

    def save(self, name):
        OUT.mkdir(exist_ok=True)
        frames = self.frame + 2
        (OUT / f"{name}.json").write_text(json.dumps({"frames": frames, "events": self.events}, indent=1) + "\n")
        return name, frames


def scenarios():
    yield Replay().tap(HOME_CARD[1]).tap(MODE[1]).tap(SIDE[0]).save("setup_uttt_vs_computer")
    yield Replay().tap(HOME_CARD[2]).tap(RIGHT).save("rules_dandelions")

    # Ultimate TTT, two players: X centre->top-right, O there->centre, then X opens the centre board.
    r = Replay().tap(HOME_CARD[1]).tap(START)
    for board, cell in [(4, 2), (2, 4), (4, 0), (0, 4), (4, 8), (8, 4)]:
        r.tap(ut_board(board)).tap(ut_cell(cell))
    yield r.save("uttt_overview")
    r = Replay().tap(HOME_CARD[1]).tap(START)
    for board, cell in [(4, 2), (2, 4), (4, 0), (0, 4)]:
        r.tap(ut_board(board)).tap(ut_cell(cell))
    yield r.tap(ut_board(4)).save("uttt_zoomed")
    # X wins board 4 with cells 0, 4 and 8; O is then sent to the won board 4 and may play anywhere.
    r = Replay().tap(HOME_CARD[1]).tap(START)
    for board, cell in [(4, 0), (0, 4), (4, 8), (8, 4), (4, 4)]:
        r.tap(ut_board(board)).tap(ut_cell(cell))
    yield r.save("uttt_small_win_free_move")
    # Against the computer (X human, medium).
    r = Replay().tap(HOME_CARD[1]).tap(MODE[1]).tap(START)
    r.tap(ut_board(4)).tap(ut_cell(4), wait=AI_WAIT)
    yield r.save("uttt_vs_computer")

    # Order & Chaos: Order completes five Xs across the top row in two-player mode.
    r = Replay().tap(HOME_CARD[0]).tap(START)
    for order, chaos in [(0, 30), (1, 31), (2, 32), (3, 34)]:
        r.tap(oc_cell(order)).tap(OC_MARK["O"]).tap(oc_cell(chaos)).tap(OC_MARK["X"])
    yield r.tap(oc_cell(4)).save("oc_order_wins")
    r = Replay().tap(HOME_CARD[0]).tap(MODE[1]).tap(LEVEL[2]).tap(START)
    for cell in [14, 15, 21]:
        r.tap(oc_cell(cell), wait=AI_WAIT)
    yield r.save("oc_vs_hard_computer")

    # Dandelions, two players: plant, blow, plant, ending on the wind's turn.
    r = Replay().tap(HOME_CARD[2]).tap(START)
    for cell, wind in [(6, "E"), (18, "N"), (2, "SW")]:
        r.tap(da_cell(cell)).tap(da_dir(wind))
    yield r.tap(da_cell(24)).save("dandelions_wind_turn")
    r = Replay().tap(HOME_CARD[2]).tap(MODE[1]).tap(SIDE[1]).tap(LEVEL[2]).tap(START).wait(AI_WAIT)
    for wind in ["N", "E", "S", "W", "NE", "SE", "SW"]:
        r.tap(da_dir(wind), wait=AI_WAIT)
    yield r.save("dandelions_full_game_vs_computer")


if __name__ == "__main__":
    for name, frames in scenarios():
        print(f"{name} {frames}")
