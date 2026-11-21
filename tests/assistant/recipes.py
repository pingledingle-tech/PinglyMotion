"""
The worked recipes of docs/ASSISTANT_GUIDE.md, section 7, run as tests (part of issue 129).

The guide is the source: every recipe there is a fenced block after a `<!-- recipe:<id> -->`
marker, one JSON command per line. This file reads those lines, sends them to a real PinglyMotion
exactly as written, and checks
  - every command answers ok,
  - the expectations below on the answers (check_animation: no errors and exactly the warnings
    the guide talks about; export_anim: loop, priority, joints written),
  - the acceptance checks of lib.run_check on the saved .dingle, opened again,
  - the exported .anim file, read back byte by byte (header, joints, loop points),
  - and that an untouched animation fails the acceptance checks, so they are not empty.
So a changed command or a changed answer breaks the test, and the guide is fixed with it.

Run by run_reference.py (CI); alone: tests/assistant/recipes.py path/to/PinglyMotion
"""

import json
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import lib  # noqa: E402

GUIDE = os.path.join(lib.ROOT, "docs", "ASSISTANT_GUIDE.md")
MARKER = re.compile(r"<!-- recipe:([a-z_]+) -->\s*```\n(.*?)```", re.S)

NO_ERRORS = {"type": "check_animation"}

# Expectations per recipe. "warnings": the exact set of check names check_animation may warn
# about (the guide explains each). "anim": what the exported file must hold.
EXPECT = {
    "wave": {
        "checks": [
            {"type": "duration", "seconds": [1.4, 1.6]},
            {"type": "pose", "expect": "rHand.y@2 > head.y@2 + 3"},
            {"type": "pose", "expect": "rHand.y@9 > head.y@9 + 2"},
            # the forearm swings the hand side to side (x), not forward
            {"type": "pose", "expect": "rHand.x@9 > rHand.x@2 + 5"},
            {"type": "pose", "expect": "rHand.z@9 < rHand.z@2 + 2"},
            {"type": "moves", "joints": ["rForeArm"]},
            {"type": "motion", "expect": "joints.length == 1"},
            NO_ERRORS,
        ],
        "warnings": [],
        "anim": {"priority": 3, "loop": False, "ease_in": 0.3, "ease_out": 0.5, "joints": ["mShoulderRight", "mElbowRight"]},
    },
    "sit": {
        "checks": [
            {"type": "pose", "expect": "hip.y@last < 30"},
            {"type": "pose", "expect": "hip.y@last > 26"},
            {"type": "pose", "expect": "lFoot.z@last > hip.z@last + 15"},
            {"type": "pose", "expect": "rFoot.z@last > hip.z@last + 15"},
            {"type": "pose", "expect": "lFoot.above_floor@last < 6"},
            {"type": "pose", "expect": "rFoot.above_floor@last < 6"},
            {"type": "pose", "expect": "lHand.y@last < hip.y@last + 12"},
            {"type": "changed", "joints": ["lThigh", "rThigh", "lShin", "rShin", "lForeArm", "rForeArm"]},
            NO_ERRORS,
        ],
        "warnings": [],
        "anim": {"priority": 4, "loop": True, "ease_in": 0.5, "ease_out": 0.5,
                 "joints": ["mPelvis", "mHipLeft", "mHipRight", "mKneeLeft", "mKneeRight", "mElbowLeft", "mElbowRight"],
                 "position": "mPelvis"},
    },
    "idle": {
        "checks": [
            {"type": "duration", "seconds": [3.9, 4.1]},
            {"type": "pose", "expect": "lHand.y@2 < hip.y@2 + 8"},
            {"type": "pose", "expect": "rHand.y@2 < hip.y@2 + 8"},
            {"type": "angle", "joint": "head", "axis": "y", "frame": 41, "min": 10, "max": 14},
            {"type": "angle", "joint": "head", "axis": "y", "frame": 121, "min": -0.5, "max": 0.5},
            {"type": "moves", "joints": ["head"]},
            {"type": "loop_seam", "allowed": ["smooth"]},
            {"type": "slide", "max_speed": 0.5},
            {"type": "motion", "expect": "loop.loop_out == 121"},
            NO_ERRORS,
        ],
        "warnings": [],
        "anim": {"priority": 2, "loop": True, "ease_in": 0.3, "ease_out": 0.3,
                 "joints": ["mChest", "mHead", "mShoulderLeft", "mShoulderRight"], "loop_frames": [2, 121]},
    },
    "walk": {
        "checks": [
            {"type": "duration", "seconds": [1.6, 1.8]},
            {"type": "moves", "joints": ["lThigh", "rThigh", "lShin", "rShin"]},
            {"type": "motion", "expect": "rhythm.arms == move in opposition"},
            NO_ERRORS,
        ],
        # motion capture: past joint limits a little, feet slide in place, the clip's own last step
        "warnings": ["foot_sliding", "joint_limits", "loop_seam"],
        "anim": {"priority": 3, "loop": True, "ease_in": 0.3, "ease_out": 0.3,
                 "joints": ["mHipLeft", "mHipRight", "mKneeLeft", "mKneeRight"]},
        "find": {"id": "m2m-walk-relaxed"},
    },
}


def guide_recipes(path=GUIDE):
    """[(id, [command dicts])] in the order of the guide."""
    with open(path, encoding="utf-8") as file:
        text = file.read()
    found = []
    for name, block in MARKER.findall(text):
        commands = [json.loads(line) for line in block.splitlines() if line.strip()]
        found.append((name, commands))
    return found


def read_anim(path):
    """The header and the joints of a Second Life .anim file (version 1.0)."""
    with open(path, "rb") as file:
        data = file.read()
    at = [0]

    def take(fmt):
        value = struct.unpack_from("<" + fmt, data, at[0])
        at[0] += struct.calcsize("<" + fmt)
        return value if len(value) > 1 else value[0]

    def text():
        end = data.index(b"\0", at[0])
        value = data[at[0]:end].decode("ascii")
        at[0] = end + 1
        return value

    anim = {"version": take("HH"), "priority": take("i"), "duration": take("f"), "emote": text()}
    anim["loop_in"], anim["loop_out"], anim["loop"] = take("f"), take("f"), bool(take("i"))
    anim["ease_in"], anim["ease_out"], anim["hand_pose"] = take("f"), take("f"), take("I")
    joints = {}
    for _ in range(take("I")):
        name = text()
        take("i")                                    # joint priority
        rotations = take("i")
        at[0] += rotations * 8
        positions = take("i")
        at[0] += positions * 8
        joints[name] = {"rotations": rotations, "positions": positions}
    anim["joints"] = joints
    anim["constraints"] = take("i")
    anim["rest"] = len(data) - at[0]
    return anim


def check_anim(path, want, fps, export):
    problems = []
    anim = read_anim(path)
    close = lambda a, b: abs(a - b) < 0.01  # noqa: E731
    if anim["version"] != (1, 0):
        problems.append("version %r" % (anim["version"],))
    for field in ("priority", "loop"):
        if anim[field] != want[field]:
            problems.append("%s is %r, wanted %r" % (field, anim[field], want[field]))
    for field in ("ease_in", "ease_out"):
        if not close(anim[field], want[field]):
            problems.append("%s is %.3f, wanted %s" % (field, anim[field], want[field]))
    if not close(anim["duration"], export["seconds"]):
        problems.append("duration %.3f differs from export_anim's seconds %s" % (anim["duration"], export["seconds"]))
    if anim["hand_pose"] != 1:                       # Relaxed, the default
        problems.append("hand pose %d, wanted 1 (Relaxed)" % anim["hand_pose"])
    missing = [name for name in want["joints"] if name not in anim["joints"]]
    if missing:
        problems.append("joints missing in the file: %s" % ", ".join(missing))
    if len(anim["joints"]) != export["joints_written"]:
        problems.append("%d joints in the file, export_anim said %d" % (len(anim["joints"]), export["joints_written"]))
    if want.get("position") and not anim["joints"].get(want["position"], {}).get("positions"):
        problems.append("no position keys on %s" % want["position"])
    if want.get("loop_frames"):
        # .anim stores loop points in seconds from frame 2: the export leaves out frame 1, the
        # reference pose, so frame 2 is 0 s
        start, end = [(frame - 2) / float(fps) for frame in want["loop_frames"]]
        if not (close(anim["loop_in"], start) and close(anim["loop_out"], end)):
            problems.append("loop %.3f to %.3f s, wanted %.3f to %.3f" % (anim["loop_in"], anim["loop_out"], start, end))
    if anim["rest"] != 0:
        problems.append("%d bytes left after the constraints" % anim["rest"])
    return problems


def run_recipe(app, name, commands):
    """Returns (problems, skipped)."""
    want = EXPECT.get(name)
    if want is None:
        return ["recipe %s has no expectations in tests/assistant/recipes.py" % name], []
    for needed in ("check_animation", "export_anim", "describe_motion", "find_reference", "set_loop"):
        if needed not in app.commands:
            return [], ["command %s is not in this build" % needed]
    problems = []
    answers = {}
    saved = exported = None
    for command in commands:
        arguments = dict(command)
        verb = arguments.pop("command")
        answer = app.run(verb, **arguments)
        if not answer.get("ok"):
            return ["%s %s failed: %s" % (verb, json.dumps(arguments)[:120], answer.get("error"))], []
        answers.setdefault(verb, []).append(answer)
        if verb == "set_pose" and answer.get("clamped"):
            problems.append("set_pose was cut back: %s" % answer.get("clamped"))
        if verb == "save_file":
            saved = arguments["path"]
        if verb == "export_anim":
            exported = (arguments, answer)

    if "find" in want:
        hits = [hit["id"] for hit in answers["find_reference"][0]["hits"]]
        if want["find"]["id"] not in hits:
            problems.append("find_reference does not list %s (hits: %s)" % (want["find"]["id"], hits))
        reference = answers["load_reference"][0].get("reference", {})
        if not reference.get("credit") or not reference.get("licence"):
            problems.append("load_reference gave no credit or licence")

    check = answers["check_animation"][-1]
    if lib.find_errors(check):
        problems.append("check_animation reports errors: %s" % check.get("summary"))
    warned = sorted({f["check"] for f in check.get("findings", []) if f.get("severity") == "warning"})
    if warned != sorted(want["warnings"]):
        problems.append("check_animation warns about %s, the guide expects %s" % (warned, sorted(want["warnings"])))

    if exported is None or saved is None:
        return problems + ["the recipe must end with save_file and export_anim"], []
    arguments, export = exported
    for field in ("priority", "loop"):
        if export.get(field) != want["anim"][field]:
            problems.append("export_anim %s is %r" % (field, export.get(field)))
    fps = app.ask("state")["fps"]
    problems += ["exported file: " + p for p in check_anim(app.path(arguments["path"]), want["anim"], fps, export)]

    failures, skipped = lib.check_saved(app, saved, want["checks"])
    problems += failures
    return problems, skipped


def untouched_fails(app, name):
    """An animation where nothing was done must fail the acceptance checks of the recipe."""
    app.reset()
    checks = [c for c in EXPECT[name]["checks"] if c["type"] != "check_animation"]
    problems, _ = lib.run_checks(app, checks)
    return bool(problems)


def library_environment():
    """The bundled library of the checkout, and no downloaded clips, so find_reference answers the
    same everywhere. Call before lib.App starts the program."""
    import tempfile
    os.environ.setdefault("PINGLYMOTION_LIBRARY_FOLDER", os.path.join(lib.ROOT, "library"))
    os.environ.setdefault("PINGLYMOTION_REFERENCE_FOLDER", tempfile.mkdtemp(prefix="pinglymotion-no-downloads-"))


def run_all(app, report):
    """report(name, problems, skips) as in run_reference.py."""
    recipes = guide_recipes()
    names = [name for name, _ in recipes]
    report("recipes: the guide has a block for every expectation",
           ["missing in the guide: %s" % n for n in EXPECT if n not in names]
           + ["duplicate recipe %s" % n for n in set(names) if names.count(n) > 1], [])
    for name, commands in recipes:
        app.reset()
        problems, skips = run_recipe(app, name, commands)
        report("recipe %s: runs as written and passes" % name, problems, skips)
        if name in EXPECT:
            report("recipe %s: an untouched animation fails" % name,
                   [] if untouched_fails(app, name) else ["it passes"], [])


def main():
    library_environment()
    app = lib.App(sys.argv[1])
    failed = []

    def report(name, problems, skips):
        print(("OK    " if not problems else "FAIL  ") + name)
        for line in problems:
            print("        " + line)
        for line in skips:
            print("        skipped " + line)
        if problems:
            failed.append(name)

    try:
        run_all(app, report)
    finally:
        app.close()
    print("ALL OK" if not failed else "FAILED: " + ", ".join(failed))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
