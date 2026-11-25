#!/usr/bin/env python3
"""Tests for the user manual (docs/MANUAL.md), its web page and its PDF.

Run: python3 tests/test_site_manual.py
"""

import os
import re
import sys
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
DOCS = os.path.join(ROOT, "docs")
sys.path.insert(0, os.path.join(ROOT, "tools", "site"))

import build_manual as bm  # noqa: E402

MANUAL = os.path.join(DOCS, "MANUAL.md")
HTML = os.path.join(DOCS, "manual.html")
PDF = os.path.join(DOCS, "PinglyMotion-Manual.pdf")

IMAGE_RE = re.compile(r"!\[([^\]]*)\]\(([^)\s]+)\)")

CHAPTERS = [
    "Start here",
    "Quick start: the first five minutes",
    "What is finished and what is not",
    "Chapter 1: Install and start",
    "Chapter 2: The window in one picture",
    "Chapter 3: Your first animation",
    "Chapter 4: Posing in detail",
    "Chapter 5: The timeline, keys and playback",
    "Chapter 6: The 3D view and the camera",
    "Chapter 7: Props",
    "Chapter 8: The library",
    "Chapter 9: Files and formats",
    "Chapter 10: Script Lab for beginners",
    "Chapter 11: AI assistants",
    "Chapter 12: The live link to Aurora",
    "Chapter 13: Setup, themes and help",
    "Chapter 14: Keyboard shortcuts and mouse",
    "Chapter 15: Every menu item",
    "Chapter 16: Troubleshooting and feedback",
    "Chapter 17: Credits",
    "Glossary",
]


def read(path):
    with open(path, encoding="utf-8") as f:
        return f.read()


class ManualText(unittest.TestCase):
    def setUp(self):
        self.text = read(MANUAL)

    def test_headings_exist(self):
        headings = [m.group(2) for m in (bm.HEADING_RE.match(l) for l in self.text.splitlines()) if m]
        for name in CHAPTERS:
            self.assertIn(name, headings, "missing heading: " + name)

    def test_every_chapter_has_an_example(self):
        # every numbered chapter has the word Example or a numbered list of steps
        parts = re.split(r"^## ", self.text, flags=re.M)
        for part in parts:
            m = re.match(r"Chapter (\d+):", part)
            if m and int(m.group(1)) <= 15:
                title = part.splitlines()[0]
                has = "**Example" in part or "### Step 1" in part or "Make the example project" in part
                self.assertTrue(has, "no example in: " + title)

    def test_images_exist_and_have_alt_text(self):
        images = IMAGE_RE.findall(self.text)
        self.assertGreaterEqual(len(images), 15)
        for alt, path in images:
            self.assertTrue(alt.strip(), "image without alt text: " + path)
            self.assertGreater(len(alt.split()), 2, "alt text too short: " + path)
            full = os.path.join(DOCS, path)
            self.assertTrue(os.path.isfile(full), "image missing: " + path)
            self.assertLess(os.path.getsize(full), 200 * 1024, "image over 200 KB: " + path)
            with open(full, "rb") as f:
                self.assertEqual(f.read(8), b"\x89PNG\r\n\x1a\n", "not a PNG: " + path)

    def test_no_unused_images(self):
        used = {os.path.basename(p) for _, p in IMAGE_RE.findall(self.text)}
        folder = os.path.join(DOCS, "manual", "img")
        for name in os.listdir(folder):
            self.assertIn(name, used, "image not used in the manual: " + name)

    def test_no_email_or_personal_paths(self):
        self.assertIsNone(re.search(r"[\w.+-]+@[\w-]+\.[\w.-]+", self.text), "an email address")
        self.assertIsNone(re.search(r"[A-Za-z]:\\Users\\(?!<)", self.text), "a Windows user path")
        self.assertIsNone(re.search(r"/(home|Users)/(?!<)[\w.-]+", self.text), "a home folder path")
        self.assertNotIn("Co-Authored-By", self.text)

    def test_internal_links_point_at_headings(self):
        anchors = {bm.slug(m.group(2)) for m in (bm.HEADING_RE.match(l) for l in self.text.splitlines()) if m}
        for target in re.findall(r"\]\(#([^)]+)\)", self.text):
            self.assertIn(target, anchors, "broken link #" + target)

    def test_status_words_are_used(self):
        for word in ("Built", "Partly built", "Trial", "Not built", "Not tested in Second Life"):
            self.assertIn(word, self.text)

    def test_credits_link(self):
        self.assertIn("(../CREDITS.md)", self.text)
        self.assertTrue(os.path.isfile(os.path.join(ROOT, "CREDITS.md")))

    def test_files_named_in_examples_exist(self):
        for name in ("wave.dingle", "jump.dingle", "jump_slow.dingle", "throne.dingle", "throne.prp"):
            self.assertIn(name, self.text)
            self.assertTrue(os.path.isfile(os.path.join(ROOT, "examples", name)), name)
        self.assertTrue(os.path.isdir(os.path.join(ROOT, "examples", "improve", "OpenCollar")))


class Converter(unittest.TestCase):
    def test_heading_and_slug(self):
        out = "\n".join(bm.parse_blocks(["## Chapter 1: Install and start"]))
        self.assertIn('id="chapter-1-install-and-start"', out)

    def test_table(self):
        out = "\n".join(bm.parse_blocks(["| A | B |", "|---|---|", "| 1 | `x` |"]))
        self.assertIn("<th>A</th>", out)
        self.assertIn("<td><code>x</code></td>", out)

    def test_nested_list_and_image(self):
        out = "\n".join(bm.parse_blocks(["1. One", "", "   ![alt text here](a.png)", "", "   - nested", "2. Two"]))
        self.assertEqual(out.count("<ol>"), 1)
        self.assertIn('<img src="a.png" alt="alt text here"', out)
        self.assertIn("<ul><li>nested</li></ul>", out)

    def test_html_is_escaped(self):
        out = bm.inline("a <script> & b")
        self.assertNotIn("<script>", out)

    def test_repo_links(self):
        self.assertIn("github.com/pingledingle-tech/PinglyMotion/blob/main/CREDITS.md", bm.inline("[c](../CREDITS.md)"))


class GeneratedFiles(unittest.TestCase):
    def test_html_is_up_to_date(self):
        self.assertEqual(read(HTML), bm.build_html(read(MANUAL)),
                         "docs/manual.html is out of date: run python3 tools/site/build_manual.py")

    def test_html_images_and_alt(self):
        page = read(HTML)
        imgs = re.findall(r"<img [^>]*>", page)
        self.assertGreaterEqual(len(imgs), 15)
        for tag in imgs:
            self.assertRegex(tag, r'alt="[^"]{5,}"')
            src = re.search(r'src="([^"]+)"', tag).group(1)
            self.assertTrue(os.path.isfile(os.path.join(DOCS, src)), src)
        self.assertNotIn("<script", page)

    def test_pdf_is_a_valid_pdf(self):
        self.assertTrue(os.path.isfile(PDF), "docs/PinglyMotion-Manual.pdf is missing")
        size = os.path.getsize(PDF)
        self.assertGreater(size, 50 * 1024)
        self.assertLess(size, 6 * 1024 * 1024)
        with open(PDF, "rb") as f:
            data = f.read()
        self.assertTrue(data.startswith(b"%PDF-"))
        self.assertIn(b"%%EOF", data[-2048:])
        self.assertGreaterEqual(len(re.findall(rb"/Type\s*/Page[^s]", data)), 10)
        self.assertIsNone(re.search(rb"/(home|Users)/[\w.-]+", data), "a home folder path inside the PDF")


if __name__ == "__main__":
    unittest.main()
