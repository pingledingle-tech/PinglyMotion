#!/usr/bin/env python3
"""Make bin/data/firestorm/ from a checkout of the Firestorm viewer (#88).

Reads, from the Firestorm source (LGPL 2.1):
  indra/newview/fsposeranimator.h                         PoserJoints table
  indra/newview/skins/default/xui/en/floater_fs_poser.xml joint titles, headings,
                                                          axis transform strings
  indra/newview/app_settings/poses/hand_presets/*.xml     the 9 hand presets
and Pingmator's bin/data/bento_joints.txt (BVH name <-> SL name).

Writes bin/data/firestorm/joints.json and copies the hand presets unchanged
into bin/data/firestorm/hand_presets/.

Usage: python3 tools/make_firestorm_joints.py <firestorm checkout> <commit>
Run from the repository root. Licensed under the GNU GPL version 2 or later;
the data it writes stays under the LGPL 2.1 (see bin/data/firestorm/README.md).
"""

import json
import re
import shutil
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

OUT = Path("bin/data/firestorm")

# Pingmator's own names for joints Firestorm's poser shows without a title
PINGMATOR_NAMES = {
    "mFaceRoot": "Face Root",
    "mFaceChin": "Chin",
}

# joints of Pingmator's skeleton that Firestorm's PoserJoints does not list
# (name, mirror partner); a Firestorm title, when there is one, wins
PINGMATOR_EXTRA = {
    "mFaceEyeAltLeft": ("Left Eye (alternative)", "mFaceEyeAltRight"),
    "mFaceEyeAltRight": ("Right Eye (alternative)", "mFaceEyeAltLeft"),
    "mFaceJawShaper": ("Jaw Shaper", ""),
}
PINGMATOR_EXTRA_SECTION = {
    "mFaceEyeAltLeft": "Eyes/Eye Lids",
    "mFaceEyeAltRight": "Eyes/Eye Lids",
    "mFaceJawShaper": "Cheeks/Lips",
}


def split_top(text):
    """Split at commas that are not inside braces or quotes."""
    parts, depth, cur, quoted = [], 0, "", False
    for ch in text:
        if ch == '"':
            quoted = not quoted
        elif not quoted and ch == "{":
            depth += 1
        elif not quoted and ch == "}":
            depth -= 1
        if ch == "," and depth == 0 and not quoted:
            parts.append(cur.strip())
            cur = ""
        else:
            cur += ch
    if cur.strip():
        parts.append(cur.strip())
    return parts


def poser_joints(header):
    text = header.read_text(encoding="utf-8")
    body = text[text.index("PoserJoints{") + len("PoserJoints{"):]
    body = body[:body.index("};")]
    body = re.sub(r"//[^\n]*", "", body)
    entries, depth, start = [], 0, 0
    for i, ch in enumerate(body):
        if ch == "{":
            if depth == 0:
                start = i + 1
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                entries.append(body[start:i])
    joints = []
    for entry in entries:
        f = split_top(entry)
        name = f[0].strip('"')
        mirror = f[1].strip('"')
        btype = f[2]
        dont_flip = len(f) > 6 and f[6] == "true"
        joints.append({"sl": name, "mirror": mirror, "type": btype, "dontFlip": dont_flip})
    return joints


def floater_strings(xml):
    root = ET.parse(xml).getroot()
    return {s.get("name"): (s.text or "").strip() for s in root.iter("string")}


def tab_of(bvh):
    """Same rules as JointGroups::tabOf() (src/jointgroups.cpp)."""
    if bvh.startswith("mHand"):
        return "Hands"
    if bvh == "mGroin":
        return "Other"
    if bvh.startswith(("mFace", "mEye")) or bvh == "mSkull":
        return "Face"
    if bvh.startswith(("mWing", "mTail", "mHindLimb")):
        return "Other"
    return "Body"


def main():
    fs = Path(sys.argv[1])
    commit = sys.argv[2]
    bvh_of = {}
    for line in Path("bin/data/bento_joints.txt").read_text().splitlines():
        if line.startswith("#") or not line.strip():
            continue
        f = line.split()
        bvh_of[f[1]] = f[0]

    strings = floater_strings(fs / "indra/newview/skins/default/xui/en/floater_fs_poser.xml")
    joints = []
    section, last_type = "", None
    for j in poser_joints(fs / "indra/newview/fsposeranimator.h"):
        if j["type"] == "COL_VOLUMES":
            continue  # collision volumes are not BVH joints
        if j["type"] != last_type:
            section, last_type = "", j["type"]
        section = strings.get("header_" + j["sl"], section)
        sl = j["sl"]
        if sl not in bvh_of:
            sys.exit(f"{sl} is not in bento_joints.txt")
        entry = {
            "sl": sl,
            "bvh": bvh_of[sl],
            "name": strings.get("title_" + sl, ""),
            "nameFrom": "firestorm",
            "tab": tab_of(bvh_of[sl]),
            "firestormType": j["type"],
            "firestormSection": section,
            "mirror": j["mirror"],
            "dontFlipOnMirror": j["dontFlip"],
        }
        if not entry["name"]:
            if sl not in PINGMATOR_NAMES:
                sys.exit(f"Firestorm has no title for {sl}: add it to PINGMATOR_NAMES")
            entry["name"], entry["nameFrom"] = PINGMATOR_NAMES[sl], "pingmator"
        for key, prefix in (("firestormTransform", "joint_transform_"),
                            ("firestormFrame", "joint_frame_"),
                            ("firestormDeltaRotate", "joint_delta_rotate_")):
            if prefix + sl in strings:
                value = strings[prefix + sl]
                entry[key] = (value == "true") if key == "firestormDeltaRotate" else value
        joints.append(entry)

    known = {j["sl"] for j in joints}
    for sl, bvh in bvh_of.items():
        if sl in known:
            continue
        if sl not in PINGMATOR_EXTRA:
            sys.exit(f"no name for {sl}: add it to PINGMATOR_EXTRA")
        name, mirror = PINGMATOR_EXTRA[sl]
        name_from = "pingmator"
        if "title_" + sl in strings:
            name, name_from = strings["title_" + sl], "firestorm"
        joints.append({
            "sl": sl, "bvh": bvh, "name": name, "nameFrom": name_from,
            "tab": tab_of(bvh), "firestormType": "", "firestormSection": PINGMATOR_EXTRA_SECTION[sl],
            "mirror": mirror, "dontFlipOnMirror": sl.endswith("Right"),
        })

    presets_src = fs / "indra/newview/app_settings/poses/hand_presets"
    (OUT / "hand_presets").mkdir(parents=True, exist_ok=True)
    presets = []
    for xml in sorted(presets_src.glob("*.xml")):
        shutil.copyfile(xml, OUT / "hand_presets" / xml.name)
        presets.append({"name": xml.stem.replace("_", " "), "file": "hand_presets/" + xml.name})

    data = {
        "about": "Joint names, sections, mirror partners and hand presets for the posing UX (#88). "
                 "Taken from the Firestorm viewer; see README.md in this folder.",
        "licence": "GNU Lesser General Public License version 2.1 only, see LICENSE.txt in this folder",
        "copyright": "Phoenix Firestorm Viewer Source Code, Copyright (c) 2024 Angeldark Raymaker @ Second Life; "
                     "Copyright (C) Linden Research, Inc.",
        "source": {
            "repository": "https://github.com/FirestormViewer/phoenix-firestorm",
            "commit": commit,
            "files": [
                "indra/newview/fsposeranimator.h",
                "indra/newview/skins/default/xui/en/floater_fs_poser.xml",
                "indra/newview/app_settings/poses/hand_presets/*.xml",
            ],
        },
        "joints": joints,
        "handPresets": presets,
    }
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / "joints.json").write_text(json.dumps(data, indent=1, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"{len(joints)} joints, {len(presets)} hand presets")


if __name__ == "__main__":
    main()
