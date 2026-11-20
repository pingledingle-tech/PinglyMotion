#!/usr/bin/env python3
"""Make a slower copy of a .dingle example animation.

Every key frame, the loop points and the frame count are multiplied by the
same factor (rounded half up), so the motion runs slower. Key values, ease
flags, easing choices, the frame time, the skeleton and the figure are kept.
Frame 0 stays frame 0.

Usage: python3 tools/slow_example.py examples/jump.dingle examples/jump_slow.dingle [--factor 2.5]

Standard library only. The output is JSON with 2-space indentation and a
trailing newline, so running the script again gives the same file.
"""

import argparse
import json
import sys


def scale_frame(frame, factor):
    """Scale one frame number; rounds half up and keeps frame 0 at 0."""
    return int(frame * factor + 0.5)


def slow_down(data, factor):
    """Return a new dict with all frame numbers scaled by factor."""
    if factor < 1:
        raise ValueError("factor must be 1 or more (a slower copy)")
    out = json.loads(json.dumps(data))  # deep copy, keeps key order

    timing = out["timing"]
    timing["frames"] = scale_frame(timing["frames"], factor)

    loop = out.get("loop")
    if loop:
        for name in ("in", "out"):
            if loop[name] >= 0:
                loop[name] = scale_frame(loop[name], factor)

    for track in out["tracks"]:
        for kind in ("rotation", "position"):
            for key in track.get(kind, []):
                key["frame"] = scale_frame(key["frame"], factor)

    last = max(
        (key["frame"] for track in out["tracks"]
         for kind in ("rotation", "position") for key in track.get(kind, [])),
        default=0,
    )
    if last >= timing["frames"]:
        raise ValueError("a key frame lies outside the new frame count")
    return out


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("source")
    parser.add_argument("target")
    parser.add_argument("--factor", type=float, default=2.5)
    args = parser.parse_args(argv)

    with open(args.source, encoding="utf-8") as f:
        data = json.load(f)
    slow = slow_down(data, args.factor)
    with open(args.target, "w", encoding="utf-8", newline="\n") as f:
        f.write(json.dumps(slow, indent=2, ensure_ascii=False) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
