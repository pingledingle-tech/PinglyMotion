#!/usr/bin/env python3
"""Checks the Script Lab example folders and their guides (job E1, docs/EXAMPLES.md).

- every examples/objects/*/example.json and examples/objects/guide_now.json
  has the documented format, unique ids and known parts;
- every example has one to five steps; each step names a control that
  exists: a text of the program (tr(), QT_TRANSLATE_NOOP or a .ui string)
  or a button of the example's own llDialog; the step's text names it;
- every prop has a shape of Prop::PropType (src/prop.h), a joint of both
  skeletons (bin/data/SLFemale.bvh, SLMale.bvh) or none, three numbers for
  position, scale and rotation, and the name of a prim of the folder;
- the texts in the files and in the tables of docs/EXAMPLES.md are the same,
  in the same order, and the page passes tools/check_texts.py;
- "What the program can do now" is the first section of docs/EXAMPLES.md,
  and the shapes it lists are exactly those of src/prop.h.

Standard library only. Licensed under the GNU GPL version 2 or later.
"""

import glob
import json
import os
import re
import subprocess
import sys
import unittest

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
OBJECTS = os.path.join(ROOT, "examples", "objects")
DOC = os.path.join(ROOT, "docs", "EXAMPLES.md")
EXAMPLE_PARTS = {"what", "word", "piece", "step", "see", "next"}
NOW_PARTS = {"what", "shape", "how", "link", "effects", "open", "not"}
ORDER = ["First_colour", "Button_HUD", "Gear_set", "Timed_lock", "RLV_relay", "Work_system"]


def read(path):
    with open(path, encoding="utf-8") as fh:
        return fh.read()


def prop_shapes():
    """The names of Prop::PropType in src/prop.h, as written there (Box, Sphere, ...)."""
    text = read(os.path.join(ROOT, "src", "prop.h"))
    body = re.search(r"typedef enum\s*\{([^}]*)\}\s*PropType;", text).group(1)
    return [m.group(1) for m in re.finditer(r"(\w+)\s*=\s*\d+", body)]


def joints(skeleton):
    text = read(os.path.join(ROOT, "bin", "data", skeleton))
    return set(re.findall(r"^\s*(?:ROOT|JOINT)\s+(\S+)", text, re.M))


def program_texts():
    """Every text the program shows: tr("..."), QT_TRANSLATE_NOOP(ctx, "...") and .ui <string>s."""
    texts = set()
    for path in glob.glob(os.path.join(ROOT, "src", "**", "*.*"), recursive=True):
        if not path.endswith((".cpp", ".h", ".ui")):
            continue
        text = read(path)
        texts.update(re.findall(r'\btr\("((?:[^"\\]|\\.)*)"', text))
        texts.update(re.findall(r'QT_TRANSLATE_NOOP\(\s*"[^"]*"\s*,\s*"((?:[^"\\]|\\.)*)"', text))
        texts.update(re.findall(r"<string>([^<]*)</string>", text))
    return texts


def dialog_buttons(folder):
    """The buttons of every llDialog in the example's scripts."""
    buttons = set()
    for path in glob.glob(os.path.join(folder, "**", "*.lsl"), recursive=True):
        for m in re.finditer(r"llDialog\([^\[]*\[([^\]]*)\]", read(path)):
            buttons.update(re.findall(r'"([^"]*)"', m.group(1)))
    return buttons


def prim_names(folder):
    """The prims of the folder model: Root (or the object's name), then every other subfolder."""
    name = os.path.basename(folder)
    subs = sorted(d for d in os.listdir(folder) if os.path.isdir(os.path.join(folder, d)) and not d.startswith("."))
    root = [d for d in subs if d.lower() == "root"]
    return {root[0] if root else name} | {d for d in subs if d.lower() != "root"}


def register_rows():
    """{id: text} and the ids in order, from the tables of docs/EXAMPLES.md."""
    rows = {}
    order = []
    for line in read(DOC).splitlines():
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        if line.startswith("|") and len(cells) == 6 and re.match(r"^EX[A-Z]+-\d{2}$", cells[0]):
            rows[cells[0]] = cells[3]
            order.append(cells[0])
    return rows, order


def examples():
    out = []
    for name in ORDER:
        with open(os.path.join(OBJECTS, name, "example.json"), encoding="utf-8") as fh:
            out.append((name, json.load(fh)))
    return out


def now_guide():
    with open(os.path.join(OBJECTS, "guide_now.json"), encoding="utf-8") as fh:
        return json.load(fh)


class ExamplesTest(unittest.TestCase):
    def test_every_example_folder_is_listed(self):
        found = sorted(os.path.basename(os.path.dirname(p)) for p in glob.glob(os.path.join(OBJECTS, "*", "example.json")))
        folders = sorted(d for d in os.listdir(OBJECTS) if os.path.isdir(os.path.join(OBJECTS, d)))
        self.assertEqual(found, sorted(ORDER))
        self.assertEqual(folders, sorted(ORDER))
        self.assertEqual([e["order"] for _, e in examples()], list(range(len(ORDER))))

    def check_guide(self, guide, parts, prefix):
        ids = [g["id"] for g in guide]
        self.assertEqual(ids, ["%s-%02d" % (prefix, i) for i in range(1, len(ids) + 1)])
        for g in guide:
            self.assertIn(g["part"], parts, g["id"])
            self.assertTrue(g["text"].strip(), g["id"])

    def test_format_and_props(self):
        shapes = [s.lower() for s in prop_shapes()]
        female, male = joints("SLFemale.bvh"), joints("SLMale.bvh")
        prefixes = set()
        for name, e in examples():
            with self.subTest(example=name):
                self.assertEqual(e["format"], 1)
                self.assertEqual(e["object"], name)
                self.assertIsInstance(e["works_now"], bool)
                self.assertTrue(e["title"])
                prefix = e["guide"][0]["id"].split("-")[0]
                self.assertNotIn(prefix, prefixes)
                prefixes.add(prefix)
                self.check_guide(e["guide"], EXAMPLE_PARTS, prefix)
                for part in ("what", "step", "see", "next"):
                    self.assertTrue(any(g["part"] == part for g in e["guide"]), part)
                prims = prim_names(os.path.join(OBJECTS, name))
                self.assertTrue(e["props"])
                for p in e["props"]:
                    self.assertEqual(sorted(p), ["joint", "name", "position", "rotation", "scale", "shape"])
                    self.assertIn(p["name"], prims)
                    self.assertIn(p["shape"], shapes)
                    if p["joint"]:
                        self.assertIn(p["joint"], female, p["name"])
                        self.assertIn(p["joint"], male, p["name"])
                    for field in ("position", "scale", "rotation"):
                        self.assertEqual(len(p[field]), 3)
                        self.assertTrue(all(isinstance(v, (int, float)) for v in p[field]))
                    self.assertTrue(all(v > 0 for v in p["scale"]))
                self.assertEqual(len({p["name"] for p in e["props"]}), len(e["props"]))

    def test_gear_set_pieces(self):
        # the owner's main example: the pieces, their shapes and joints (23 Nov 2026, 21:17)
        gear = dict(examples())["Gear_set"]
        got = {p["name"]: (p["shape"], p["joint"]) for p in gear["props"]}
        self.assertEqual(got, {
            "Root": ("box", ""),
            "Wrist_left": ("torus", "lHand"), "Wrist_right": ("torus", "rHand"),
            "Ankle_left": ("torus", "lFoot"), "Ankle_right": ("torus", "rFoot"),
            "Collar": ("torus", "neck"), "Hood": ("cone", "head"),
            "Gag": ("sphere", "head"), "Blindfold": ("torus", "head"),
        })
        # the blindfold is a flat ring, the HUD a flat box
        sizes = {p["name"]: p["scale"] for p in gear["props"]}
        self.assertLess(sizes["Blindfold"][2], sizes["Blindfold"][0] / 4)
        self.assertLess(sizes["Root"][2], sizes["Root"][0] / 4)

    def test_steps_name_controls_that_exist(self):
        texts = program_texts()
        for name, e in examples():
            with self.subTest(example=name):
                steps = [g for g in e["guide"] if g["part"] == "step"]
                self.assertTrue(1 <= len(steps) <= 5, len(steps))
                buttons = dialog_buttons(os.path.join(OBJECTS, name))
                for s in steps:
                    control = s["control"]
                    self.assertTrue(control in texts or control in buttons, "%s: %s" % (s["id"], control))
                    self.assertIn(control.rstrip("."), s["text"], s["id"])
                    for field in s.get("values", {}):
                        self.assertIn(field, texts, s["id"])
                        self.assertIn(field, s["text"], s["id"])

    def test_texts_match_the_page(self):
        rows, order = register_rows()
        ids = []
        for guide in [now_guide()["guide"]] + [e["guide"] for _, e in examples()]:
            for g in guide:
                ids.append(g["id"])
                self.assertEqual(rows.get(g["id"]), g["text"], g["id"])
        self.assertEqual(order, ids)

    def test_page_passes_check_texts(self):
        result = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "check_texts.py"), DOC],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout)

    def test_what_the_program_can_do_now(self):
        headings = re.findall(r"^## (.+)$", read(DOC), re.M)
        self.assertEqual(headings[0], "What the program can do now")
        titles = [e["title"] for _, e in examples()]
        self.assertEqual(headings[1:1 + len(titles)], ["%d. %s" % (i, t) for i, t in enumerate(titles)])
        now = now_guide()
        self.assertEqual(now["format"], 1)
        self.assertEqual(now["title"], "What the program can do now")
        self.check_guide(now["guide"], NOW_PARTS, "EXNOW")
        listed = [g["name"] for g in now["guide"] if g["part"] == "shape"]
        self.assertEqual(listed, prop_shapes())
        for g in now["guide"]:
            if g["part"] == "shape":
                self.assertTrue(g["text"].startswith(g["name"]), g["id"])
        for part in NOW_PARTS:
            self.assertTrue(any(g["part"] == part for g in now["guide"]), part)


if __name__ == "__main__":
    unittest.main(verbosity=1)
