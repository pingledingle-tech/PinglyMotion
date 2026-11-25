#!/usr/bin/env python3
"""The clickable mockup (docs/mockup/index.html) shows the program as built (since 25 Nov 2026): it says so,
it has no proposal marks and no side tabs, it draws what the program has (the Animate / Script switch, the
helper, Script Lab tabs that pop out, Convert to SLua, the guide page), and its program texts (the M2T
table) are the same as the rows in docs/TEACHING.md.
Run: python3 tests/test_site_mockup_m2.py"""

import html
import json
import os
import re
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
DOCS = os.path.join(HERE, "..", "docs")
MARK = "The program as built, 25 Nov 2026"


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
    def test_marked_as_built(self):
        page = read("mockup", "index.html")
        self.assertIn(MARK, page)
        self.assertIn("The program as built, 25 Nov 2026", read("UX_DECISIONS.md"))
        self.assertNotIn("PROPOSAL", page)
        self.assertNotIn("Questions for the project owner", page)
        self.assertNotIn("Approved mockup", page)

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

    def test_register_has_no_extra_mockup_rows(self):
        texts = mockup_texts()
        section = read("TEACHING.md").split("### Mockup texts (")[1].split("\n### ")[0]
        ids = re.findall(r"^\| ([A-Z]+-\d{2}) \|", section, re.M)
        self.assertEqual(sorted(ids), sorted(texts))

    def test_only_token_colours_in_the_as_built_css(self):
        page = read("mockup", "index.html")
        start = page.index("/* ---- The program as built (25 Nov 2026)")
        css = page[start:page.index("</style>", start)]
        self.assertIsNone(re.search(r"#[0-9A-Fa-f]{3,8}\b", css))

    def test_what_is_not_built_is_gone(self):
        page = read("mockup", "index.html")
        for gone in ("m2strip", "m2toggle", "data-m2place", "data-m2tab", "m2Hud", "m2hud", "shapefold", "nexthint", "Cut begin", 'id="m2HelpBtn"'):
            self.assertNotIn(gone, page, gone)

    def test_what_the_program_has_is_drawn(self):
        page = read("mockup", "index.html")
        for there in ('id="sideAnimate"', 'id="sideScript"', 'id="helperBtn"', 'id="helper"', 'id="resetLayoutBtn"', 'data-cmd="paneltoggle"',
                      'data-cmd="showjoints"', 'data-cmd="showexpert"', 'data-m2do="popout"', 'data-m2do="putback"', 'data-m2do="convert"',
                      'data-cmd="guide"', 'id="dlgGuide"', "Make the example folder", "Convert to SLua", "Make your first move", "Arrange the windows", "Try the Script Lab"):
            self.assertIn(there, page, there)
        # the five Script Lab tabs, in the order of the program, and no HUD tab
        tabs = re.search(r"const M2_PAGES = \[(.*?)\];", page, re.S).group(1)
        order = [m for m in re.findall(r'\["(live|commands|map|player|improve|hud)"', tabs)]
        self.assertEqual(order, ["live", "commands", "map", "player", "improve"])
        self.assertIn('case "hcnext": advanceCard(); return;', page)


if __name__ == "__main__":
    unittest.main()
