#!/usr/bin/env python3
"""Compare two BVH files: joint names, channels, frame count, frame time and values.

Standard library only, no Blender needed. Use it before and after a trip
through Blender (see docs/BLENDER.md, "Scripted round trip"), or on any two
BVH files that should hold the same motion.

What is compared:
- joint names and their order, parent of each joint, channel lists
- number of frames and frame time
- joint offsets (bone positions), within --tol
- every frame value, within --tol; rotation channels are compared as angles,
  so 180 and -180 degrees count as equal

Usage:
  bvh_compare.py ORIGINAL.bvh OTHER.bvh [--tol 0.001] [--time-tol 0.00001] [--max-report 20]

Exit code 0 when the files match, 1 when they differ, 2 when a file cannot be read.
"""
import argparse
import sys


class BvhError(Exception):
    pass


class Bvh:
    def __init__(self):
        self.joints = []      # list of (name, parent index or -1, offset tuple, channels list)
        self.frames = 0
        self.frame_time = 0.0
        self.data = []        # list of frames, each a list of floats


def parse(text):
    """Parse BVH text into a Bvh. Raises BvhError on a file that is not valid."""
    bvh = Bvh()
    lines = [ln.strip() for ln in text.replace("\r", "").split("\n")]
    pos = 0
    n = len(lines)

    def next_line():
        nonlocal pos
        while pos < n and not lines[pos]:
            pos += 1
        if pos >= n:
            raise BvhError("file ends too early")
        ln = lines[pos]
        pos += 1
        return ln

    if next_line().upper() != "HIERARCHY":
        raise BvhError("HIERARCHY missing")
    stack = []            # parent indexes of the open joint blocks
    pending = None        # joint waiting for its "{"
    end_site_depth = 0
    while True:
        ln = next_line()
        words = ln.split()
        key = words[0]
        if key in ("ROOT", "JOINT"):
            if len(words) < 2:
                raise BvhError("joint without a name")
            parent = stack[-1] if stack else -1
            bvh.joints.append([words[1], parent, (0.0, 0.0, 0.0), []])
            pending = len(bvh.joints) - 1
        elif key == "End":
            pending = -2
        elif key == "{":
            if pending is None:
                raise BvhError("unexpected {")
            stack.append(pending)
            pending = None
        elif key == "}":
            if not stack:
                raise BvhError("unexpected }")
            stack.pop()
            if not stack:
                break
        elif key == "OFFSET":
            if stack and stack[-1] >= 0:
                try:
                    bvh.joints[stack[-1]][2] = tuple(float(w) for w in words[1:4])
                except ValueError:
                    raise BvhError("bad OFFSET: " + ln)
        elif key == "CHANNELS":
            if not stack or stack[-1] < 0:
                raise BvhError("CHANNELS outside a joint")
            try:
                count = int(words[1])
            except (IndexError, ValueError):
                raise BvhError("bad CHANNELS: " + ln)
            names = words[2:]
            if len(names) != count:
                raise BvhError("CHANNELS count does not match: " + ln)
            bvh.joints[stack[-1]][3] = names
        else:
            raise BvhError("unknown line in HIERARCHY: " + ln)
    if next_line().upper() != "MOTION":
        raise BvhError("MOTION missing")
    ln = next_line()
    if not ln.lower().startswith("frames:"):
        raise BvhError("Frames: missing")
    try:
        bvh.frames = int(ln.split(":", 1)[1].split()[0])
    except (IndexError, ValueError):
        raise BvhError("bad Frames line")
    ln = next_line()
    if not ln.lower().startswith("frame time:"):
        raise BvhError("Frame Time: missing")
    try:
        bvh.frame_time = float(ln.split(":", 1)[1].split()[0])
    except (IndexError, ValueError):
        raise BvhError("bad Frame Time line")
    width = sum(len(j[3]) for j in bvh.joints)
    for i in range(bvh.frames):
        try:
            row = [float(w) for w in next_line().split()]
        except ValueError:
            raise BvhError("frame %d has a value that is not a number" % i)
        if len(row) != width:
            raise BvhError("frame %d has %d values, expected %d" % (i, len(row), width))
        bvh.data.append(row)
    return bvh


def load(path):
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            return parse(f.read())
    except OSError as e:
        raise BvhError("cannot read %s: %s" % (path, e.strerror))


def angle_diff(a, b):
    d = (a - b) % 360.0
    return min(d, 360.0 - d)


def compare(a, b, tol=0.001, time_tol=0.00001, max_report=20):
    """Return a list of difference messages; an empty list means the files match."""
    out = []

    names_a = [j[0] for j in a.joints]
    names_b = [j[0] for j in b.joints]
    if names_a != names_b:
        missing = [x for x in names_a if x not in names_b]
        extra = [x for x in names_b if x not in names_a]
        if missing:
            out.append("joints missing in the second file: " + ", ".join(missing))
        if extra:
            out.append("joints only in the second file: " + ", ".join(extra))
        if not missing and not extra:
            out.append("joints are in a different order")
        return out  # values cannot be compared without the same joints

    for i, (ja, jb) in enumerate(zip(a.joints, b.joints)):
        pa = names_a[ja[1]] if ja[1] >= 0 else "(none)"
        pb = names_b[jb[1]] if jb[1] >= 0 else "(none)"
        if pa != pb:
            out.append("joint %s hangs under %s, was %s" % (ja[0], pb, pa))
        if ja[3] != jb[3]:
            out.append("joint %s channels differ: %s / %s" % (ja[0], " ".join(ja[3]), " ".join(jb[3])))
        for k in range(3):
            if abs(ja[2][k] - jb[2][k]) > tol:
                out.append("joint %s offset differs: %s / %s" % (ja[0], ja[2], jb[2]))
                break
    if out:
        return out

    if a.frames != b.frames:
        out.append("frame count differs: %d / %d" % (a.frames, b.frames))
    if abs(a.frame_time - b.frame_time) > time_tol:
        out.append("frame time differs: %g / %g (%.3f / %.3f fps)" % (
            a.frame_time, b.frame_time,
            1.0 / a.frame_time if a.frame_time else 0.0,
            1.0 / b.frame_time if b.frame_time else 0.0))
    if a.frames != b.frames:
        return out

    labels = []
    for j in a.joints:
        for ch in j[3]:
            labels.append((j[0], ch))
    shown = 0
    bad = 0
    worst = 0.0
    for f in range(a.frames):
        for c, (x, y) in enumerate(zip(a.data[f], b.data[f])):
            rot = labels[c][1].lower().endswith("rotation")
            d = angle_diff(x, y) if rot else abs(x - y)
            if d > tol:
                bad += 1
                worst = max(worst, d)
                if shown < max_report:
                    out.append("frame %d, %s %s: %g / %g" % (f, labels[c][0], labels[c][1], x, y))
                    shown += 1
    if bad:
        out.append("%d values differ by more than %g (largest difference %g)" % (bad, tol, worst))
    return out


def main(argv):
    ap = argparse.ArgumentParser(description="Compare two BVH files.")
    ap.add_argument("original")
    ap.add_argument("other")
    ap.add_argument("--tol", type=float, default=0.001, help="largest allowed difference in values and offsets")
    ap.add_argument("--time-tol", type=float, default=0.00001, help="largest allowed difference in frame time")
    ap.add_argument("--max-report", type=int, default=20, help="most single values to list")
    args = ap.parse_args(argv[1:])
    try:
        a = load(args.original)
        b = load(args.other)
    except BvhError as e:
        print("error: %s" % e, file=sys.stderr)
        return 2
    diffs = compare(a, b, args.tol, args.time_tol, args.max_report)
    if diffs:
        print("DIFFERENT")
        for d in diffs:
            print("  " + d)
        return 1
    print("SAME: %d joints, %d frames, frame time %g" % (len(a.joints), a.frames, a.frame_time))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
