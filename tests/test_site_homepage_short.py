#!/usr/bin/env python3
"""The short homepage: summary and download button come first, the page stays short,
every language has the real release link, and no old "on its way" wording is left.
Run: python3 tests/test_site_homepage_short.py"""

import html
import os
import re
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
DOCS = os.path.join(HERE, "..", "docs")
LANGS = ["en", "no", "de", "pl", "nl"]
RELEASE = "https://github.com/pingledingle-tech/PinglyMotion/releases/tag/v0.3.0-beta.1"


def read(name):
    with open(os.path.join(DOCS, name), encoding="utf-8") as f:
        return f.read()


def article(page, lang):
    return re.search(r'<article id="%s".*?</article>' % lang, page, re.S).group(0)


def words(a):
    a = re.sub(r"<nav.*?</nav>|<footer>.*?</footer>", " ", a, flags=re.S)
    return len(html.unescape(re.sub(r"<[^>]+>", " ", a)).split())


class HomepageShort(unittest.TestCase):
    def test_summary_and_button_first(self):
        page = read("index.html")
        for lang in LANGS:
            a = article(page, lang)
            btn = re.search(r'<a class="btn" href="%s">[^<]+</a>' % re.escape(RELEASE), a)
            self.assertIsNotNone(btn, lang)
            self.assertLess(a.index('class="tagline'), btn.start(), lang)
            self.assertLess(btn.start(), a.index("<h2"), lang)

    def test_short(self):
        page = read("index.html")
        for lang in LANGS:
            self.assertLess(words(article(page, lang)), 400, lang)

    def test_english_and_norwegian_text(self):
        page = read("index.html")
        en, no = article(page, "en"), article(page, "no")
        for needle in ("What it does.", "No AI was harmed in the making of this tool.", "a lot of Lindens", "Pingle is a she"):
            self.assertIn(needle, en)
        for needle in ("Hva den gjør.", "Pingle er en hun", "mange Lindens"):
            self.assertIn(needle, no)

    def test_no_old_wording(self):
        for name in ("index.html", "download.html"):
            page = read(name)
            for old in ("is on its way", "er på vei", "still being prepared", "noch vorbereitet", "jeszcze przygotowywane", "nog voorbereid", "fortsatt under arbeid"):
                self.assertNotIn(old, page, (name, old))

    def test_download_page_links_release(self):
        page = read("download.html")
        for lang in LANGS:
            self.assertIn(RELEASE, article(page, lang), lang)

    def test_headings_in_order(self):
        page = read("index.html")
        for lang in LANGS:
            a = article(page, lang)
            self.assertNotIn("<h1", a)
            self.assertNotIn("<h3", a)
            self.assertGreaterEqual(a.count("<h2"), 1, lang)


if __name__ == "__main__":
    unittest.main()
