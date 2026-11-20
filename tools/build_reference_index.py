#!/usr/bin/env python3
"""Build library/ (the bundled reference library, issue 19) from the source clips.

Not run in CI (it needs the unchanged source files). The result is committed;
tests/test_reference_library.py checks it. Format: docs/REFERENCE_LIBRARY_SCHEMA.md.

Sources (all three must be present on this machine):
  --cmu DIR     the data/ folder of una-dinosauria/cmu-mocap (cgspeed BVH
                conversion), commit 09a07f54f3bbb58797325f009282d0b2048a2871
  --m2m DIR     static/assets/human/animations of Mesh2Motion/mesh2motion-app,
                commit bcb014ea25e44f76b3add78463f72d1faec16418
  --glb2sl PATH the built tool glb2sl_bvh (cmake target glb2sl_bvh)

Usage: build_reference_index.py --cmu DIR --m2m DIR --glb2sl PATH [--out library]
Standard library only.
"""
import argparse
import hashlib
import json
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")

TAGS = ["walk", "run", "jump", "sit", "stand", "idle", "dance", "gesture", "wave", "talk",
        "drink", "sport", "fight", "kneel", "crouch", "lie", "emote", "happy", "sad", "angry",
        "calm", "slow", "fast", "turn", "backwards", "loop", "tool", "work", "transition", "music"]

SOURCES = {
    "cmu": {
        "name": "CMU Graphics Lab Motion Capture Database",
        "url": "http://mocap.cs.cmu.edu",
        "licence": "CMU: no restrictions on use (UNCERTAIN: no formal licence text; see docs/REFERENCE_LIBRARY_PLAN.md)",
        "credit": "The data used in this project was obtained from mocap.cs.cmu.edu. "
                  "The database was created with funding from NSF EIA-0196217.",
        "about_name": "CMU Graphics Lab Motion Capture Database",
        "origin": "cgspeed.com BVH conversion (2010) as copied in una-dinosauria/cmu-mocap, "
                  "commit 09a07f54f3bbb58797325f009282d0b2048a2871; converted with tools/cmu_to_sl.py",
    },
    "mesh2motion": {
        "name": "Mesh2Motion",
        "url": "https://github.com/Mesh2Motion/mesh2motion-app",
        "licence": "CC0 1.0",
        "credit": "Animation from Mesh2Motion (github.com/Mesh2Motion/mesh2motion-app), CC0 1.0.",
        "about_name": "Mesh2Motion",
        "origin": "Mesh2Motion/mesh2motion-app commit bcb014ea25e44f76b3add78463f72d1faec16418, "
                  "static/assets/human/animations; converted with glb2sl_bvh",
    },
}

# id, file/trial, start, end (seconds, CMU only), name, description, tags
CMU = [
    ("07_04", None, None, "Slow walk", "A slow, relaxed walk forward.", ["walk", "slow", "calm"]),
    ("07_12", None, None, "Brisk walk", "A quick, purposeful walk forward.", ["walk", "fast"]),
    ("37_01", None, None, "Slow walk, long", "A slow walk that goes on for a few steps.", ["walk", "slow"]),
    ("16_11", None, None, "Walk and veer left", "A walk that curves to the left.", ["walk", "turn"]),
    ("16_30", None, None, "Walk, turn right", "A walk with a 90 degree turn to the right.", ["walk", "turn"]),
    ("111_01", None, None, "Walk backwards", "Walks backwards.", ["walk", "backwards"]),
    ("02_03", None, None, "Jog", "A short jog.", ["run"]),
    ("16_08", None, None, "Jog and stop", "A jog that ends in a sudden stop.", ["run", "transition"]),
    ("75_01", None, None, "Run and jump", "A run that ends in a jump.", ["run", "jump"]),
    ("16_05", None, None, "Forward jump", "A standing jump forward.", ["jump"]),
    ("118_01", None, None, "Jumps", "A few jumps in place.", ["jump"]),
    ("05_03", None, None, "Ballet, sideways steps", "Sideways arabesque, a turn step, folding arms.", ["dance", "calm"]),
    ("05_06", None, None, "Ballet, pirouettes", "A cartwheel-like start, pirouettes and a jete.", ["dance"]),
    ("05_12", None, None, "Ballet, arms high", "Arms held high, a pointed foot, upper body rotation.", ["dance", "calm"]),
    ("60_01", 0.0, 8.0, "Salsa", "Salsa dancing, eight seconds.", ["dance", "music"]),
    ("10_01", None, None, "Kick a ball", "Walks up and kicks a soccer ball.", ["sport"]),
    ("13_09", None, None, "Drink soda", "Lifts a can and drinks from it.", ["drink", "stand"]),
    ("14_37", None, None, "Drink soda, short", "A shorter version of drinking from a can.", ["drink", "stand"]),
    ("79_01", None, None, "Chop wood", "Swings an axe to chop wood.", ["tool", "work"]),
    ("80_01", None, None, "Shake hands", "Reaches out and shakes hands with someone.", ["gesture"]),
    ("77_01", None, None, "Look around", "Stands and looks around.", ["stand", "idle"]),
    ("13_04", 2.0, 9.5, "Sit on a stool, chin in hand", "Sits down on a stepstool and rests the chin in the hand.", ["sit", "calm"]),
    ("14_30", 2.0, 10.0, "Sit on a stool, ankle on knee", "Sits on a stepstool with one ankle on the other knee.", ["sit", "calm"]),
    ("13_14", 0.0, 8.0, "Laugh", "Standing, laughing.", ["emote", "happy"]),
    ("13_17", 0.0, 8.0, "Boxing", "Boxing moves: jabs and footwork.", ["fight"]),
    ("13_27", 3.0, 11.0, "Direct traffic", "Waves and points like a traffic guard.", ["gesture", "wave"]),
    ("18_08", 0.0, 10.0, "Explain with gestures", "Talks and explains with hand gestures.", ["talk", "gesture"]),
    ("18_10", 0.0, 10.0, "Angry gestures", "An argument with angry hand gestures.", ["talk", "gesture", "angry"]),
]

# file stem, name, description, tags
M2M = [
    ("sitting-enter", "Sit down", "Lowers into a chair.", ["sit", "transition"]),
    ("sitting-exit", "Stand up from chair", "Gets up from a chair.", ["sit", "transition"]),
    ("sitting-talking", "Sit and talk", "Seated, talking with small gestures.", ["sit", "talk"]),
    ("sitting-writing", "Sit and write", "Seated, writing.", ["sit", "work"]),
    ("sit-ground-idle", "Sit on the ground", "Sits on the ground, idle.", ["sit", "idle", "calm"]),
    ("sit-ground-crying", "Sit on the ground, crying", "Sits on the ground and cries.", ["sit", "sad", "emote"]),
    ("crouch-idle", "Crouch", "Crouching, idle.", ["crouch", "idle"]),
    ("kneel", "Kneel", "Kneels down.", ["kneel"]),
    ("idle-foldarms", "Stand, arms folded", "Standing with folded arms.", ["stand", "idle"]),
    ("idle-listening", "Stand and listen", "Standing, listening.", ["stand", "idle", "talk"]),
    ("idle-talking", "Stand and talk", "Standing, talking with gestures.", ["stand", "talk", "gesture"]),
    ("idle-subtle", "Stand, subtle idle", "A quiet standing idle.", ["stand", "idle", "calm"]),
    ("head-nod", "Nod", "A head nod.", ["gesture", "emote"]),
    ("clapping", "Clap", "Clapping hands.", ["gesture", "happy"]),
    ("victory", "Victory", "A victory pose and cheer.", ["emote", "happy"]),
    ("reject", "Reject", "A dismissive, rejecting gesture.", ["gesture", "emote"]),
    ("confused", "Confused", "Looks confused.", ["emote"]),
    ("angry", "Angry", "An angry outburst.", ["emote", "angry"]),
    ("meditate", "Meditate", "Meditating.", ["sit", "calm"]),
    ("pray", "Pray", "Praying.", ["kneel", "calm"]),
    ("dance-arm-pump", "Dance, arm pump", "A dance with pumping arms.", ["dance", "happy"]),
    ("dance-body-roll", "Dance, body roll", "A dance with a body roll.", ["dance"]),
    ("dance-charleston", "Dance, Charleston", "The Charleston.", ["dance"]),
    ("dance-reach-hip", "Dance, reach and hip", "A dance reaching out, hand on hip.", ["dance"]),
    ("walk-happy", "Walk, happy", "A cheerful walk, on the spot.", ["walk", "happy", "loop"]),
    ("walk-relaxed", "Walk, relaxed", "A relaxed walk, on the spot.", ["walk", "calm", "loop"]),
    ("walk-female", "Walk, female", "A walk with a female gait, on the spot.", ["walk", "loop"]),
]


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        h.update(f.read())
    return h.hexdigest()


def bvh_info(path):
    frames = fps = None
    with open(path) as f:
        for line in f:
            if line.startswith("Frames:"):
                frames = int(line.split()[1])
            elif line.startswith("Frame Time:"):
                fps = round(1.0 / float(line.split()[2]))
                break
    return frames, fps


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--cmu", required=True)
    p.add_argument("--m2m", required=True)
    p.add_argument("--glb2sl", required=True)
    p.add_argument("--out", default=os.path.join(ROOT, "library"))
    a = p.parse_args()
    for sub in ("cmu", "mesh2motion"):
        os.makedirs(os.path.join(a.out, sub), exist_ok=True)
    clips = []

    def add(source, cid, rel, name, desc, tags, origin):
        path = os.path.join(a.out, rel)
        frames, fps = bvh_info(path)
        src = SOURCES[source]
        clips.append({
            "id": cid, "name": name, "description": desc, "tags": tags,
            "source": source, "licence": src["licence"], "credit": src["credit"],
            "origin": origin, "skeleton": "sl-classic", "joints": 19,
            "frames": frames, "fps": fps, "duration_s": round((frames - 1) / fps, 2),
            "file": rel, "bytes": os.path.getsize(path), "sha256": sha256(path),
        })

    for trial, start, end, name, desc, tags in CMU:
        cid = "cmu-" + trial.replace("_", "-")
        rel = "cmu/cmu_%s.bvh" % trial
        folder = "%03d" % int(trial.split("_")[0])
        cmd = [sys.executable, os.path.join(HERE, "cmu_to_sl.py"),
               os.path.join(a.cmu, folder, trial + ".bvh"), os.path.join(a.out, rel)]
        origin = "CMU trial " + trial
        if start is not None:
            cmd += ["--start", str(start), "--end", str(end)]
            origin += ", %.1f to %.1f s" % (start, end)
        subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL)
        add("cmu", cid, rel, name, desc, tags, origin)

    for stem, name, desc, tags in M2M:
        rel = "mesh2motion/m2m_%s.bvh" % stem
        subprocess.run([a.glb2sl, os.path.join(a.m2m, stem + ".glb"), os.path.join(a.out, rel)],
                       check=True, stdout=subprocess.DEVNULL)
        add("mesh2motion", "m2m-" + stem, rel, name, desc, tags, "Mesh2Motion clip " + stem + ".glb")

    index = {"schema_version": 1, "tag_vocabulary": sorted(TAGS), "sources": SOURCES, "clips": clips}
    with open(os.path.join(a.out, "index.json"), "w") as f:
        json.dump(index, f, indent=1, ensure_ascii=False)
        f.write("\n")
    total = sum(c["bytes"] for c in clips)
    print("%d clips, %d bytes" % (len(clips), total))


if __name__ == "__main__":
    main()
