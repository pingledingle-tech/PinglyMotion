#!/usr/bin/env python3
"""Tests for tools/check_texts.py. Run: python3 tests/test_check_texts.py"""

import os
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "tools"))

import check_texts as ct  # noqa: E402

HEAD = "| ID | Place | Trigger | Text | Status | Used by |\n|---|---|---|---|---|---|\n"


def register(*rows):
    return HEAD + "".join("| %s | 1 Test | when | %s | %s | #1 |\n" % r for r in rows)


def problems_for(*rows):
    return ct.check_rows(ct.parse_register(register(*rows)))[0]


class RowTests(unittest.TestCase):
    def test_good_row(self):
        self.assertEqual(problems_for(("TIP-01", "Drag it.<br>Then press Play.", "draft")), [])

    def test_real_register_is_clean(self):
        with open(ct.REGISTER, encoding="utf-8") as fh:
            rows = ct.parse_register(fh.read())
        self.assertGreater(len(rows), 40)
        self.assertEqual(ct.check_rows(rows)[0], [])

    def test_duplicate_id(self):
        p = problems_for(("TIP-01", "One.", "draft"), ("TIP-01", "Two.", "draft"))
        self.assertTrue(any("already used" in x for x in p))

    def test_bad_id(self):
        self.assertTrue(any("ID must" in x for x in problems_for(("tip1", "One.", "draft"))))

    def test_empty_text(self):
        self.assertTrue(any("empty" in x for x in problems_for(("TIP-01", "", "draft"))))

    def test_placeholder(self):
        self.assertTrue(any("placeholder" in x for x in problems_for(("TIP-01", "TODO write this", "draft"))))

    def test_bad_status(self):
        self.assertTrue(any("status" in x for x in problems_for(("TIP-01", "One.", "maybe"))))

    def test_status_with_date_is_ok(self):
        self.assertEqual(problems_for(("TIP-01", "One.", "reviewed by the owner (19 Nov 2026)")), [])

    def test_long_sentence(self):
        text = "one two three four five six seven eight nine ten eleven twelve thirteen."
        self.assertTrue(any("13 words" in x for x in problems_for(("TIP-01", text, "draft"))))

    def test_twelve_words_ok(self):
        text = "one two three four five six seven eight nine ten eleven twelve."
        self.assertEqual(problems_for(("TIP-01", text, "draft")), [])

    def test_each_line_counts_alone(self):
        self.assertEqual(problems_for(("TIP-01", "a b c d e f g h i j.<br>k l m n o p q r s t.", "draft")), [])

    def test_avoided_words(self):
        for bad in ("Set a keyframe.", "Rotate the arm.", "Use IK here.", "Pick ease in.",
                    "Hits the joint limit.", "No Euler angles."):
            self.assertTrue(any("avoided" in x for x in problems_for(("TIP-01", bad, "draft"))), bad)

    def test_plain_words_ok(self):
        self.assertEqual(problems_for(("TIP-01", "Turn the arm. Pick Slow start.", "draft")), [])

    def test_personal_data(self):
        for bad in ("Mail me at a" + chr(64) + "b.com.", "Call 555 123 4567.", "See /home/bob/x.", "Go to https://x.org."):
            self.assertTrue(any("personal" in x for x in problems_for(("TIP-01", bad, "draft"))), bad)

    def test_wrong_column_count(self):
        text = HEAD + "| TIP-01 | only | three |\n"
        self.assertTrue(any("columns" in x for x in ct.check_rows(ct.parse_register(text))[0]))


class ReferenceTests(unittest.TestCase):
    def test_missing_reference(self):
        with tempfile.TemporaryDirectory() as d:
            with open(os.path.join(d, "a.cpp"), "w") as fh:
                fh.write('tr("TIP-01"); tr("TIP-09"); // PNG-12 is unrelated\n')
            refs = ct.find_references(d, os.path.join(d, "none.md"))
            p = ct.check_references(refs, {"TIP-01"})
        self.assertEqual(len(p), 1)
        self.assertIn("TIP-09", p[0])


if __name__ == "__main__":
    unittest.main()
