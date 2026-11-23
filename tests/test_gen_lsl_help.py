#!/usr/bin/env python3
"""Tests for tools/gen_lsl_help.py and the editor's help texts (lane 1, job F2).

- src/scriptlab/lslhelp_data.inc is what the tool makes from the copy of Linden Lab's definitions
  (so a new copy of the definitions needs a new run of the tool);
- the reader gets names, types, arguments and descriptions right on a small made-up file;
- every plain line of our own in src/scriptlab/lslhelp_plain.inc names something that exists,
  has at most 12 words a sentence, no avoided word and no placeholder (the rules of
  tools/check_texts.py).
"""
import os
import re
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import gen_lsl_help as g  # noqa: E402
import check_texts  # noqa: E402

PLAIN = os.path.join(ROOT, "src", "scriptlab", "lslhelp_plain.inc")

FAKE = """\
llsd-lsl-syntax-version: 2
constants:
  ALPHA:
    tooltip: First line
      goes on. Second sentence.
    type: integer
    value: '0x2'
  SECRET:
    private: true
    type: integer
    value: 1
events:
  tick:
    event-id: 1
    arguments:
    - n:
        tooltip: count
        type: integer
    tooltip: "Runs \\"often\\".\\nMore."
    categories:
      - timer
functions:
  llDo:
    arguments:
    - val:
        tooltip: a value
        type: float
    - name:
        tooltip: a name
        type: string
    return: integer
    tooltip: |-
      Does it, e.g. now. Then more.
    categories:
    - misc
  llOld:
    arguments: []
    deprecated: true
    tooltip: Old.
"""


def plain_lines():
    rows = []
    with open(PLAIN, encoding="utf-8") as f:
        for n, line in enumerate(f, 1):
            m = re.match(r'^(LSL_PLAIN(?:_TYPE|_KEYWORD)?)\("([^"]+)", "([^"]+)"\)\s*$', line)
            if m:
                rows.append((n, m.group(1), m.group(2), m.group(3)))
            elif line.startswith("LSL_"):
                raise AssertionError("line %d is not in the form LSL_PLAIN(\"name\", \"sentence\")" % n)
    return rows


class GenLslHelpTest(unittest.TestCase):
    def test_data_file_is_up_to_date(self):
        self.assertEqual(g.main(["--check"]), 0, "run python3 tools/gen_lsl_help.py")

    def test_reader(self):
        with tempfile.TemporaryDirectory() as d:
            path = os.path.join(d, "defs.yaml")
            with open(path, "w", encoding="utf-8") as f:
                f.write(FAKE)
            data = g.read(path)
            text = g.make(data)
        self.assertEqual(data["constants"]["ALPHA"]["tooltip"], "First line goes on. Second sentence.")
        self.assertEqual(data["functions"]["llDo"]["args"], [["val", "float"], ["name", "string"]])
        self.assertIn('LSL_HELP_FUNCTION("llDo", "integer", "float val,string name", 0, "misc", "Does it, e.g. now.")', text)
        self.assertIn('LSL_HELP_FUNCTION("llOld", "void", "", 1, "", "Old.")', text)
        self.assertIn('LSL_HELP_EVENT("tick", "integer n", "timer", "Runs \\"often\\".")', text)
        self.assertIn('LSL_HELP_CONSTANT("ALPHA", "integer", "0x2", "First line goes on.")', text)
        self.assertNotIn("SECRET", text)

    def test_first_sentence(self):
        self.assertEqual(g.first_sentence("One. Two."), "One.")
        self.assertEqual(g.first_sentence("Use it, e.g. Here. Then."), "Use it, e.g. Here.")
        self.assertEqual(g.first_sentence("No end"), "No end")
        self.assertEqual(g.first_sentence("Line\\nbreak. Next"), "Line break.")

    def test_known_entries(self):
        with open(g.OUTPUT, encoding="utf-8") as f:
            text = f.read()
        self.assertIn('LSL_HELP_FUNCTION("llOwnerSay", "void", "string msg", 0, "avatar_communication", ', text)
        self.assertIn('LSL_HELP_EVENT("touch_start", "integer num_detected", "detected", "Triggered when an avatar first touches the object.")', text)
        self.assertIn('LSL_HELP_CONSTANT("TRUE", "integer", "1", ', text)

    def test_plain_lines(self):
        with open(g.OUTPUT, encoding="utf-8") as f:
            data = f.read()
        known = set(re.findall(r'^LSL_HELP_(?:FUNCTION|EVENT|CONSTANT)\("(\w+)"', data, re.M))
        deprecated = set(re.findall(r'^LSL_HELP_FUNCTION\("(\w+)", "\w+", "[^"]*", 1,', data, re.M))
        rows = plain_lines()
        self.assertGreater(len(rows), 100)
        seen = set()
        for n, kind, name, sentence in rows:
            where = "lslhelp_plain.inc line %d (%s)" % (n, name)
            self.assertNotIn(name, seen, where + ": listed twice")
            seen.add(name)
            if kind == "LSL_PLAIN":
                self.assertIn(name, known, where + ": not in the definitions")
                self.assertNotIn(name, deprecated, where + ": deprecated, never suggested")
            self.assertTrue(sentence.endswith("."), where + ": end with a full stop")
            for s in check_texts.sentences(sentence):
                self.assertLessEqual(len(s.split()), check_texts.MAX_WORDS, where + ": over 12 words: " + s)
            for rx in check_texts.AVOIDED_RE:
                self.assertIsNone(rx.search(sentence), where + ": avoided word: " + sentence)
            self.assertIsNone(check_texts.PLACEHOLDER_RE.search(sentence), where + ": placeholder")


if __name__ == "__main__":
    unittest.main()
