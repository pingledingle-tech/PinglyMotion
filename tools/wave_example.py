#!/usr/bin/env python3
"""Make the example wave, examples/wave.dingle (issue #53 step 6).

A short loop with five keys (saved poses) on the right arm, built on the
relaxed start pose bin/data/Relaxed.dingle. Each of the four pose changes uses
a different easing choice, stored the same way a click on the tile row stores
it (docs/EASING.md, "Tile row"):

  key 1 -> key 2  arm lifts        Slow both ends         start easeOut + target easeIn (+ "curve", see below)
  key 2 -> key 3  hand swings out  Overshoot, then settle start key "curve"
  key 3 -> key 4  hand swings back Even                   no switch
  key 4 -> key 5  arm comes down   Slow finish            target easeIn only

Key 1 is at frame 0, whose ease switches the program resets to the user's
default when a file is opened; so the first pose change also stores its choice
as "curve" ("ease-in-out"), which survives that. The last key equals the
first, so the loop has no jump. The notes for the keys (WAVE-01 to WAVE-06 in
docs/TEACHING.md, reviewed by the project owner) are kept in "extensions" under
"pinglymotion.exampleNotes"; the program shows them in the notes line under
the timeline and keeps them when the file is saved (UX phase 3c;
docs/FIRST_ANIMATION.md lists them).

Usage: python3 tools/wave_example.py bin/data/Relaxed.dingle examples/wave.dingle

Standard library only. The output is JSON with 2-space indentation and a
trailing newline, so running the script again gives the same file.
"""

import argparse
import json
import sys

# Key frames (file frame numbers; frame 0 is the reference frame Second Life
# does not play, so the played loop runs from frame 1 to the last key).
KEY_FRAMES = [0, 20, 30, 40, 60]
FRAME_COUNT = 61
LOOP = {"in": 1, "out": 60}

# Pose of each moving joint per key, as [x, y, z] degrees (the order set_pose
# and the assistant guide use). Key 1 and key 5 are the relaxed pose itself.
LIFTED = {
    "rCollar": [0.0, 0.0, -12.0],
    "rShldr": [-80.0, 15.0, -35.0],
    "rForeArm": [0.0, 70.0, 0.0],
    "rHand": [0.0, 0.0, 0.0],
}
HAND_OUT = {
    "rCollar": [0.0, 0.0, -12.0],
    "rShldr": [-80.0, 15.0, -35.0],
    "rForeArm": [0.0, 25.0, 0.0],
    "rHand": [0.0, 0.0, -20.0],
}
MOVING = list(LIFTED)

# The easing of the pose change that starts at each key (None for the last key).
CHOICES = ["Slow both ends", "Overshoot, then settle", "Even", "Slow finish", None]

# How the tile row stores each choice: (start key easeOut, target key easeIn, curve).
STORED = {
    "Even": (False, False, None),
    "Slow start": (True, False, None),
    "Slow finish": (False, True, None),
    "Slow both ends": (True, True, None),
    "Overshoot, then settle": (True, True, "ease-out-overshoot"),
    "Hold, then snap": (True, True, "constant"),
}

# The technical names in .dingle (docs/DINGLE_FORMAT.md, "Easing choice").
TECHNICAL = {
    "Even": "linear",
    "Slow start": "ease-in",
    "Slow finish": "ease-out",
    "Slow both ends": "ease-in-out",
    "Overshoot, then settle": "ease-out-overshoot",
    "Hold, then snap": "constant",
}

# WAVE-01 to WAVE-05 belong to keys 1 to 5, WAVE-06 is the small task after them.
NOTES = [
    "A key is a saved pose. This one has the arm down.\nThe wave starts and ends here, so it loops.",
    "The arm lifts here.\nSlow both ends makes it start and stop softly.",
    "The hand swings out, goes a little past, then settles.\nThat is Overshoot, then settle.",
    "The hand swings back at one steady speed.\nThat is Even. It looks stiff, so try Slow both ends.",
    "The arm slows down as it lands.\nIt matches the first key, so the loop has no jump.",
]
TASK = "Your turn: make the arm stop more softly."

EXTENSION = "pinglymotion.exampleNotes"


def to_file_order(xyz, channels):
    """[x, y, z] degrees to the joint's rotation channels in file order."""
    axis = {"Xrotation": xyz[0], "Yrotation": xyz[1], "Zrotation": xyz[2]}
    return [float(axis[c]) for c in channels if c.endswith("rotation")]


def make_wave(relaxed):
    """Return the wave as a .dingle dict, built on the relaxed pose."""
    out = json.loads(json.dumps(relaxed))  # deep copy, keeps key order
    out["generator"] = "PinglyMotion"
    out["timing"]["frames"] = FRAME_COUNT
    out["loop"] = dict(LOOP)
    names = [joint["name"] for joint in out["skeleton"]]

    # every track that does not move keeps its one key at frame 0
    for track in out["tracks"]:
        for kind in ("rotation", "position"):
            for key in track.get(kind, []):
                if key["frame"] != 0:
                    raise ValueError("the start pose must have keys at frame 0 only")

    poses = {}
    for name in MOVING:
        joint = names.index(name)
        track = next(t for t in out["tracks"] if t["joint"] == joint)
        channels = out["skeleton"][joint]["channels"]
        rest = track["rotation"][0]["values"]
        poses[name] = [rest, to_file_order(LIFTED[name], channels),
                       to_file_order(HAND_OUT[name], channels),
                       to_file_order(LIFTED[name], channels), rest]

        keys = []
        for i, frame in enumerate(KEY_FRAMES):
            keys.append({"frame": frame, "values": [float(v) for v in poses[name][i]],
                         "easeIn": False, "easeOut": False})
        for i, choice in enumerate(CHOICES):
            if choice is None:
                continue
            ease_out, ease_in, curve = STORED[choice]
            keys[i]["easeOut"] = ease_out
            keys[i + 1]["easeIn"] = ease_in
            if curve is None and KEY_FRAMES[i] == 0:
                # opening a file sets the ease switches of every frame 0 key to
                # the user's default (off unless changed), but keeps its "curve";
                # so the first pose change names its choice explicitly
                curve = TECHNICAL[choice]
            if curve:
                keys[i]["curve"] = {"type": curve}
        track["rotation"] = keys

    out["extensions"] = {
        EXTENSION: {
            "keys": [
                {"frame": frame, "choice": CHOICES[i], "note": NOTES[i]}
                for i, frame in enumerate(KEY_FRAMES)
            ],
            "task": TASK,
        }
    }
    return out


def tidy(value):
    """Write whole floats as integers, as the bundled examples do."""
    if isinstance(value, float) and value.is_integer():
        return int(value)
    if isinstance(value, list):
        return [tidy(v) for v in value]
    if isinstance(value, dict):
        return {k: tidy(v) for k, v in value.items()}
    return value


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("source", help="the relaxed start pose (bin/data/Relaxed.dingle)")
    parser.add_argument("target", help="the wave to write (examples/wave.dingle)")
    args = parser.parse_args(argv)

    with open(args.source, encoding="utf-8") as f:
        relaxed = json.load(f)
    wave = tidy(make_wave(relaxed))
    with open(args.target, "w", encoding="utf-8", newline="\n") as f:
        f.write(json.dumps(wave, indent=2, ensure_ascii=False) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
