# SPDX-License-Identifier: GPL-3.0-or-later
"""Check the shop collection log and panel in the running game.

One scripted run (tools/drive_game.py) with RECOMP_COLLECTION_LOG=1 and
RECOMP_EXTRA_OVERLAYS=1: it walks to the Sports Shop, moves the cursor and buys
an item, and checks the [auto:collection] lines against the [auto:ui] lines
and against each other. The run's saves are put back afterwards. Prints one
[auto:check] line per check; the exit code is the number that failed. See
docs/collection-log.md."""
import argparse
from pathlib import Path
import re
import sys
import time

import drive_game
from drive_game import DriveError, Session, say
from extract_iso import package_root

# The entries the cursor visits, from the shop's first highlighted one.
CURSOR_STEPS = (("down", 3), ("down", 4), ("up", 3))
# What a log line has to say. Groups are named for the checks below.
UI_SHOP = r"\[auto:ui\] frame \d+: shop (?P<shop>\d+) cursor (?P<cursor>\d+) item (?P<item>\d+)$"
COLLECTION_SHOP = (r"\[auto:collection\] frame \d+: shop {shop} cursor {cursor}: "
                   r"item {item} (?P<name>.*) \(type \d+\):(?P<owners>.*)$")
PANEL = r"\[auto:collection\] frame \d+: panel: (?P<text>.*); lit: (?P<lit>.*)$"


def find(lines, start, pattern):
    """The match of the first line from index `start` on, or None."""
    expression = re.compile(pattern)
    for line in lines[start:]:
        match = expression.search(line)
        if match:
            return match
    return None


def index_of(lines, pattern, start=0):
    expression = re.compile(pattern)
    return next((index for index in range(start, len(lines)) if expression.search(lines[index])), None)


def owners(text):
    """{"Tina": "none", ...} from the collection log's shop line."""
    return {name: state.split(",")[0] for name, state in re.findall(r" (\w+)=([\w+,-]+)", text)}


class Checks:
    def __init__(self, session):
        self.session = session
        self.failed = 0

    def report(self, name, passed, detail):
        print(f"[auto:check] {'PASS' if passed else 'FAIL'} {name}: {detail}", flush=True)
        if not passed:
            self.failed += 1
        return passed

    def wait_for(self, start, pattern, timeout=5.0):
        """The first match from line `start` on, waiting up to `timeout` s."""
        deadline = time.monotonic() + timeout
        while True:
            match = find(self.session.lines(), start, pattern)
            if match is not None or time.monotonic() >= deadline:
                return match
            time.sleep(drive_game.POLL)

    def save_load(self):
        lines = self.session.lines()
        # The collection log runs before the UI log in a frame, so its summary
        # can come just ahead of the "save vacation 1" line.
        chosen = index_of(lines, r"\[auto:ui\] .*menu screen save-select$")
        island = index_of(lines, r"\[auto:ui\] .*scene island$", chosen or 0)
        summary = find(lines[:island], chosen or 0, r"\[auto:collection\] .*items changed; per girl")
        self.report("save-load", chosen is not None and island is not None and summary is not None,
                    "the save select read the vacation's ownership table" if summary
                    else "no ownership summary between the save select and the island")

    def cursor(self, step, button, entry):
        """Moves the shop cursor; the collection log must name the same item."""
        start = len(self.session.lines())
        self.session.press_until(button, rf"shop 0 cursor {entry} item ", 10, 0.5)
        shown = re.search(UI_SHOP, self.session.latest(UI_SHOP))
        listed = self.wait_for(start, COLLECTION_SHOP.format(**shown.groupdict()))
        self.report(f"shop-cursor-{step}", listed is not None,
                    f"item {shown['item']} {listed['name']}" if listed
                    else f"no collection line for item {shown['item']} at cursor {entry}")
        if listed is None:
            return None
        panel = self.wait_for(start, PANEL.replace("(?P<text>.*)", re.escape(listed["name"]) + "(?P<text>.*)"))
        self.report(f"panel-{step}", panel is not None,
                    f"{listed['name']}{panel['text']}; lit: {panel['lit']}" if panel
                    else f"the panel never showed {listed['name']}")
        return entry, shown["item"], listed["name"], owners(listed["owners"])

    def purchase(self, item, name, before):
        start = len(self.session.lines())
        self.session.buy()
        slot = self.wait_for(start, rf"\[auto:collection\] .*girl (?P<girl>\d+) \((?P<who>\w+)\) "
                                    rf"slot \d+: empty -> item {item} ", 10)
        if not self.report("purchase-inventory", slot is not None,
                           f"{name} went into {slot['who']}'s room" if slot
                           else f"{name} reached no girl's room"):
            return
        who = slot["who"]
        if before.get(who) == "none":
            had = self.wait_for(start, rf"\[auto:collection\] .*item {item} .*girl {slot['girl']} "
                                       rf"\({who}\): none -> had")
            self.report("purchase-ownership", had is not None,
                        f"{who} now has had {name}" if had else f"{who}'s had bit never came on")
        else:
            self.report("purchase-ownership", True,
                        f"{who} already had {name} ({before.get(who)}); nothing to change")
        panel = self.wait_for(start, PANEL.replace("(?P<text>.*)", re.escape(name) + "(?P<text>.*)"))
        lit = panel is not None and "In room: " in panel["text"] and who in panel["text"].split("In room: ")[1] \
            and "present" in panel["lit"]
        self.report("purchase-panel", lit, f"{name}{panel['text']}; lit: {panel['lit']}" if panel
                    else "the panel did not change after the purchase")


def run(session):
    checks = Checks(session)
    drive_game.route(session, "shop")
    checks.save_load()
    seen = [found for step, (button, entry) in enumerate(CURSOR_STEPS, 1)
            if (found := checks.cursor(step, button, entry)) is not None]
    if seen:
        # Buy an item no girl has had, when one was seen, so the purchase
        # also has to turn a had bit on.
        entry, item, name, before = next(
            (found for found in seen if set(found[3].values()) == {"none"}), seen[-1])
        if entry != seen[-1][0]:
            button = "down" if entry > seen[-1][0] else "up"
            session.press_until(button, rf"shop 0 cursor {entry} item ", 10, 0.5)
        checks.purchase(item, name, before)
    session.shot("collection-check")
    return checks.failed


def main(argv=None):
    root = package_root()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--session", type=Path, default=root / "private/drive")
    parser.add_argument("--runner", type=Path)
    parser.add_argument("--xbe", type=Path)
    parser.add_argument("--env", action="append", default=[], metavar="NAME=VALUE")
    args = parser.parse_args(argv)
    session = Session(args.session)
    failed = 1
    try:
        runner, image = drive_game.find_game(root, args.runner, args.xbe)
        env = dict(item.split("=", 1) for item in args.env)
        env.update(RECOMP_COLLECTION_LOG="1", RECOMP_EXTRA_OVERLAYS="1")
        session.start(runner, image, env=env)
        failed = run(session)
    except (DriveError, OSError, ValueError) as error:
        session.fail(error)
    finally:
        session.stop()
    say(f"collection checks: {'all passed' if failed == 0 else f'{failed} failed'}")
    return min(failed, 125)


if __name__ == "__main__":
    sys.exit(main())
