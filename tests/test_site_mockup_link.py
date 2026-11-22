#!/usr/bin/env python3
"""The homepage links to the clickable mockup near the top, in every language.
Run: python3 tests/test_site_mockup_link.py"""

import os
import re
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
DOCS = os.path.join(HERE, "..", "docs")
LANGS = ["en", "no", "de", "pl", "nl"]


def read(name):
    with open(os.path.join(DOCS, name), encoding="utf-8") as f:
        return f.read()


def article(html, lang):
    m = re.search(r'<article id="%s".*?</article>' % lang, html, re.S)
    assert m, lang
    return m.group(0)


class MockupLink(unittest.TestCase):
    def test_mockup_page_exists(self):
        self.assertTrue(os.path.isfile(os.path.join(DOCS, "mockup", "index.html")))

    def test_box_near_top_in_every_language(self):
        html = read("index.html")
        for lang in LANGS:
            a = article(html, lang)
            m = re.search(r'<aside class="try"[^>]*><a class="btn" href="mockup/">([^<]+)</a><p>([^<]+)</p></aside>', a)
            self.assertIsNotNone(m, lang)
            self.assertTrue(m.group(1).strip() and m.group(2).strip(), lang)
            # after the tagline, before the introduction
            self.assertLess(a.index('class="tagline'), m.start(), lang)
            self.assertLess(m.start(), a.index('aria-label="Introduction"') if lang == "en" else a.index("<section"), lang)

    def test_labels_differ_per_language(self):
        html = read("index.html")
        labels = [re.search(r'class="btn" href="mockup/">([^<]+)<', article(html, l)).group(1) for l in LANGS]
        self.assertEqual(len(set(labels)), len(LANGS))

    def test_nav_item_on_every_site_page(self):
        for page in ("index.html", "download.html", "privacy.html"):
            html = read(page)
            for lang in LANGS:
                nav = re.search(r'<article id="%s".*?<nav class="site".*?</nav>' % lang, html, re.S).group(0)
                self.assertIn('<a href="mockup/">', nav, (page, lang))

    def test_only_theme_tokens_in_new_css(self):
        css = read("site.css")
        block = css[css.index(".try{"):]
        self.assertIsNone(re.search(r"#[0-9A-Fa-f]{3,8}\b", block))


if __name__ == "__main__":
    unittest.main()
