"""
Shared code for the assistant evaluation set (part of issue 129): starts PinglyMotion with
remote control, talks to it over plain HTTP like tests/smoke_control.py, and runs the acceptance
checks of a prompt on a saved .dingle file by opening it again and asking the describe commands.

Nothing here needs a network connection or an API key.
"""

import json
import operator
import os
import re
import subprocess
import tempfile
import time
import urllib.error
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))


class Skip(Exception):
    """A check that needs a command this build does not have yet."""


class App:
    """A running PinglyMotion with --control, in a work folder of its own."""

    def __init__(self, app_path):
        self.work = tempfile.mkdtemp(prefix="pinglymotion-assistant-")
        control_file = os.path.join(self.work, "control.json")
        env = dict(os.environ, PINGLYMOTION_CONTROL_FILE=control_file,
                   PINGLYMOTION_CONTROL_FOLDER=self.work)
        self.folder = os.path.join(self.work, "files")
        os.makedirs(self.folder)
        env["PINGLYMOTION_CONTROL_FOLDER"] = self.folder
        self.process = subprocess.Popen([os.path.abspath(app_path), "--control"], env=env,
                                        cwd=os.path.join(ROOT, "bin"),
                                        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        info = None
        for _ in range(80):
            try:
                with open(control_file) as file:
                    info = json.load(file)
            except (OSError, ValueError):
                info = None
            if info and info.get("pid") == self.process.pid:
                break
            time.sleep(0.5)
        else:
            self.close()
            raise RuntimeError("PinglyMotion did not start remote control")
        self.url = info["url"]
        self.headers = {"Authorization": "Bearer " + info["token"],
                        "X-PinglyMotion-Client": "assistant eval"}
        self.commands = {c["name"] for c in self.get("/api/commands")["commands"]}

    def get(self, path):
        request = urllib.request.Request(self.url + path, headers=self.headers)
        with urllib.request.urlopen(request, timeout=30) as response:
            return json.load(response)

    def run(self, command, **arguments):
        """One command. Returns the answer; an error answer has ok false and an error text."""
        body = json.dumps(dict(arguments, command=command)).encode()
        request = urllib.request.Request(self.url + "/api", data=body, headers=self.headers)
        try:
            with urllib.request.urlopen(request, timeout=60) as response:
                return json.load(response)
        except urllib.error.HTTPError as error:
            return json.load(error)

    def need(self, command):
        if command not in self.commands:
            raise Skip("command %s is not in this build" % command)

    def ask(self, command, **arguments):
        answer = self.run(command, **arguments)
        if not answer.get("ok"):
            raise AssertionError("%s failed: %s" % (command, answer.get("error")))
        return answer

    def reset(self):
        """A new animation, with the file name of the last one forgotten."""
        self.ask("new_animation")

    def path(self, name):
        return os.path.join(self.folder, name)

    def close(self):
        if self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(timeout=15)
            except subprocess.TimeoutExpired:
                self.process.kill()


# ---------------------------------------------------------------- the checks

OPERATORS = {">": operator.gt, "<": operator.lt, ">=": operator.ge, "<=": operator.le}
TERM = re.compile(r"^\s*(?P<neg>~)?(?:(?P<path>[A-Za-z][\w.]*)(?:@(?P<frame>\w+))?)?\s*(?P<offset>[+-]?\s*[\d.]+)?\s*$")


class Session:
    """The animation that is open now, with a cache of describe_pose answers."""

    def __init__(self, app):
        self.app = app
        self.state = app.ask("state")
        self.frames = self.state["frames"]
        self.poses = {}

    def frame_number(self, word):
        if word in (None, "first"):
            return 2 if self.frames > 1 else 1
        if word == "last":
            return self.frames
        if word == "mid":
            return (2 + self.frames) // 2
        return int(word)

    def pose(self, frame):
        if frame not in self.poses:
            self.poses[frame] = self.app.ask("describe_pose", frame=frame)
        return self.poses[frame]

    def value(self, term):
        match = TERM.match(term)
        if not match:
            raise ValueError("cannot read %r" % term)
        offset = float((match.group("offset") or "0").replace(" ", ""))
        if not match.group("path"):
            return offset
        # a plain number such as "2"
        node = self.pose(self.frame_number(match.group("frame")))["positions"]
        for part in match.group("path").split("."):
            node = node[part]
        return (-node if match.group("neg") else node) + offset


def split_expression(text):
    for symbol in (">=", "<=", ">", "<"):
        if symbol in text:
            left, right = text.split(symbol, 1)
            return left, symbol, right
    raise ValueError("no comparison in %r" % text)


def find_errors(answer):
    """The findings of check_animation that Second Life would refuse (severity error). The answer
    has the counts `errors`, `warnings`, `notes` and the list `findings` (docs/CONTROL.md)."""
    problems = [f for f in answer.get("findings", []) if str(f.get("severity")).lower() == "error"]
    count = answer.get("errors", 0)
    if not problems and isinstance(count, int) and count > 0:
        problems = [{"errors": count}]
    return problems


def run_check(session, check):
    """Returns None when the check passes, otherwise a text. May raise Skip."""
    app = session.app
    kind = check["type"]
    if kind == "duration":
        seconds = (session.frames - 1) / float(session.state.get("fps") or 30)
        low, high = check["seconds"]
        if not low <= seconds <= high:
            return "duration %.2f s, wanted %s to %s" % (seconds, low, high)
    elif kind == "pose":
        # "rHand.y@mid > head.y@mid"; a frame is a number, first, mid or last
        left, symbol, right = split_expression(check["expect"])
        a, b = session.value(left), session.value(right)
        if not OPERATORS[symbol](a, b):
            return "%s is false (%.2f %s %.2f)" % (check["expect"], a, symbol, b)
    elif kind == "pose_any":
        # at some frame (every few frames from 2) one of the expressions holds; "@f" is that frame
        step = max(1, session.frames // 30)
        for frame in range(2, session.frames + 1, step):
            for text in check["any_of"]:
                left, symbol, right = split_expression(text.replace("@f", "@%d" % frame))
                if OPERATORS[symbol](session.value(left), session.value(right)):
                    return None
        return "never true: %s" % " or ".join(check["any_of"])
    elif kind == "changed":
        frame = session.frame_number(check.get("frame", "last"))
        names = [entry["joint"] for entry in session.pose(frame)["changed_joints"]]
        missing = [joint for joint in check["joints"] if joint not in names]
        if missing:
            return "frame %d: these joints did not change from frame 1: %s" % (frame, ", ".join(missing))
    elif kind == "stays":
        # a point (a foot) stays within a distance over all frames from 2: no sliding
        name, tolerance = check["point"], check["within"]
        points = [session.pose(f)["positions"][name] for f in range(2, session.frames + 1)]
        spread = max(((p["x"] - q["x"]) ** 2 + (p["z"] - q["z"]) ** 2) ** 0.5 for p in points for q in points)
        if spread > tolerance:
            return "%s moves %.2f along the floor, wanted at most %s" % (name, spread, tolerance)
    elif kind == "angle":
        # one joint angle at a frame, from get_pose, in degrees
        frame = session.frame_number(check.get("frame", "last"))
        joints = app.ask("get_pose", frame=frame, all=True)["joints"]
        angle = joints[check["joint"]]["xyz".index(check["axis"])] if isinstance(joints[check["joint"]], list) else None
        if angle is None:
            return "no angle for %s" % check["joint"]
        shown = abs(angle) if check.get("abs") else angle
        if not check.get("min", -1e9) <= shown <= check.get("max", 1e9):
            return "frame %d: %s %s is %.1f, wanted %s to %s" % (frame, check["joint"], check["axis"], angle, check.get("min"), check.get("max"))
    elif kind == "motion":
        # a value in the describe_motion answer, such as "hip.bob > 10" or "rhythm.legs == move together";
        # a list has .length
        app.need("describe_motion")
        text = check["expect"]
        symbol = "==" if "==" in text else None
        if symbol:
            left, right = [part.strip() for part in text.split("==", 1)]
        else:
            left, symbol, right = split_expression(text)
            left, right = left.strip(), float(right)
        node = app.ask("describe_motion")
        for part in left.split("."):
            node = len(node) if part == "length" else node[part]
        good = (str(node) == right) if symbol == "==" else OPERATORS[symbol](node, right)
        if not good:
            return "describe_motion: %s is false (value %r)" % (text, node)
    elif kind == "airborne":
        app.need("describe_motion")
        if not app.ask("describe_motion")["contact"]["both_feet_off_floor"]:
            return "both feet are never off the floor"
    elif kind == "moves":
        app.need("describe_motion")
        answer = app.ask("describe_motion")
        moving = [j["joint"] for j in answer["joints"]]
        missing = [joint for joint in check["joints"] if joint not in moving]
        if missing:
            return "these joints do not move: %s" % ", ".join(missing)
    elif kind == "loop_seam":
        app.need("describe_motion")
        seam = app.ask("describe_motion")["loop"]
        if seam.get("seam") not in check["allowed"]:
            return "loop seam is %r (largest %s degrees), wanted %s" % (seam.get("seam"), seam.get("largest_degrees"), check["allowed"])
    elif kind == "repeats":
        app.need("describe_motion")
        rhythm = app.ask("describe_motion")["rhythm"]
        if bool(rhythm.get("repeats")) != check["value"]:
            return "rhythm repeats is %s" % rhythm.get("repeats")
    elif kind == "slide":
        app.need("describe_motion")
        contact = app.ask("describe_motion")["contact"]
        for foot in ("lFoot", "rFoot"):
            if contact[foot]["slide_top_speed"] > check["max_speed"]:
                return "%s slides at %s inches per second, wanted at most %s" % (foot, contact[foot]["slide_top_speed"], check["max_speed"])
    elif kind == "check_animation":
        app.need("check_animation")
        problems = find_errors(app.ask("check_animation"))
        if problems:
            return "check_animation reports: %s" % json.dumps(problems)[:300]
    else:
        raise ValueError("unknown check type %r" % kind)
    return None


def run_checks(app, checks):
    """On the animation that is open now. Returns (failures, skipped) as lists of texts."""
    session = Session(app)
    failures, skipped = [], []
    for check in checks:
        try:
            problem = run_check(session, check)
        except Skip as why:
            skipped.append("%s: %s" % (check["type"], why))
            continue
        except (KeyError, AssertionError, ValueError) as error:
            problem = "%s: %r" % (check["type"], error)
        if problem:
            failures.append(problem)
    return failures, skipped


def refresh(app):
    """After open_file, frame 2 shows the pose of frame 1 (in get_pose and describe_pose) until
    some other frame has been read once (found while writing this set, see docs/DEV.md)."""
    app.ask("get_pose", frame=3, all=True)


def check_saved(app, name, checks):
    """Open the saved file again, as the user would, and run the checks on it."""
    app.reset()
    opened = app.run("open_file", path=name)
    if not opened.get("ok"):
        return ["the saved file %s does not open: %s" % (name, opened.get("error"))], []
    refresh(app)
    return run_checks(app, checks)
