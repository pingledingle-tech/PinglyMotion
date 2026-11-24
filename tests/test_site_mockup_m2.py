#!/usr/bin/env python3
"""Mockup 2 in the clickable mockup (docs/mockup/index.html) is marked as a proposal, and its
program texts (the M2T table) are the same as the rows in docs/TEACHING.md.
Run: python3 tests/test_site_mockup_m2.py"""

import html
import json
import os
import re
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
DOCS = os.path.join(HERE, "..", "docs")
MARK = "PROPOSAL, not approved until the project owner says so"


# The statuses the register allows (the "Status" line in docs/TEACHING.md).
STATUSES = ("draft", "reviewed by the owner", "in the program")


def read(*parts):
    with open(os.path.join(DOCS, *parts), encoding="utf-8") as f:
        return f.read()


def mockup_texts():
    page = read("mockup", "index.html")
    block = re.search(r"const M2T = \{\n(.*?)\n\};", page, re.S)
    assert block, "M2T not found in the mockup"
    out = {}
    for line in block.group(1).splitlines():
        m = re.match(r'\s*"([A-Z]+-\d{2})": (\[.*\]),?$', line)
        assert m, line
        out[m.group(1)] = json.loads(m.group(2))
    return out


def register_rows():
    rows = {}
    for line in read("TEACHING.md").splitlines():
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        if line.startswith("|") and len(cells) == 6 and re.match(r"^[A-Z]+-\d{2}$", cells[0]):
            rows[cells[0]] = cells
    return rows


class MockupTwo(unittest.TestCase):
    def test_marked_as_proposal(self):
        page = read("mockup", "index.html")
        self.assertIn(MARK, page)
        self.assertIn(MARK, read("UX_DECISIONS.md"))
        self.assertIn("Questions for the project owner", page)

    def test_texts_match_the_register(self):
        texts, rows = mockup_texts(), register_rows()
        self.assertGreater(len(texts), 50)
        for rid, (text, place, trigger) in texts.items():
            self.assertIn(rid, rows, rid)
            self.assertEqual(html.unescape(rows[rid][3]), text, rid)
            self.assertEqual(rows[rid][1], place, rid)
            self.assertTrue(rows[rid][4].startswith(STATUSES), rid)

    def test_register_defines_the_statuses(self):
        self.assertIn("`draft`, `reviewed by the owner`, or `in the program`", read("TEACHING.md"))

    def test_register_has_no_extra_mockup_two_rows(self):
        texts = mockup_texts()
        section = read("TEACHING.md").split("### Mockup 2 (proposal")[1].split("\n### ")[0]
        ids = re.findall(r"^\| ([A-Z]+-\d{2}) \|", section, re.M)
        self.assertEqual(sorted(ids), sorted(texts))

    def test_only_token_colours_in_mockup_two_css(self):
        page = read("mockup", "index.html")
        start = page.index("/* ---- Mockup 2 (PROPOSAL")
        css = page[start:page.index("</style>", start)]
        self.assertIsNone(re.search(r"#[0-9A-Fa-f]{3,8}\b", css))

    def test_approved_mockup_is_kept(self):
        page = read("mockup", "index.html")
        # the approved Script Lab panel and hint cards are still drawn when Mockup 2 is switched off
        self.assertIn("function renderLab() {\n  if (M2.on) { renderLab2(); return; }", page)
        self.assertIn('case "hcclose": advanceCard(); return;', page)
        self.assertIn('data-cmd="m2toggle"', page)


if __name__ == "__main__":
    unittest.main()
