#!/usr/bin/env python3
"""The front page (docs/index.html) is the joke "mare" landing page; the real homepage is docs/home.html.
The landing page starts in the mare language, has an "unmare" button, a language switcher in the order
[unmare] [MARE] [EN] [NO] [DE] [PL] [NL], all five real languages, the Windows download and release notes links,
makes no external requests and holds no personal data.
Run: python3 tests/test_site_landing.py"""

import html
import json
import os
import re
import unittest
from html.parser import HTMLParser

HERE = os.path.dirname(os.path.abspath(__file__))
DOCS = os.path.join(HERE, "..", "docs")
LANGS = ["en", "no", "de", "pl", "nl"]
RELEASE = "https://github.com/pingledingle-tech/PinglyMotion/releases/tag/v0.3.0-beta.1"
WINDOWS_ZIP = "https://github.com/pingledingle-tech/PinglyMotion/releases/download/v0.3.0-beta.1/PinglyMotion-v0.3.0-beta.1-windows.zip"


def read(*parts):
    with open(os.path.join(DOCS, *parts), encoding="utf-8") as f:
        return f.read()


def plain(fragment):
    """Text a reader of the real languages sees: the mare symbol spans are dropped."""
    fragment = re.sub(r'<span class="m" aria-hidden="true">.*?</span>', "", fragment, flags=re.S)
    return html.unescape(re.sub(r"<[^>]+>", "", fragment))


class _Collect(HTMLParser):
    """Collects the visible text (outside script and style) and the content of the script element with a given id."""

    def __init__(self, script_id=None):
        super().__init__(convert_charrefs=True)
        self.script_id, self.text, self.script, self._in = script_id, [], "", None

    def handle_starttag(self, tag, attrs):
        if tag in ("script", "style"):
            self._in = (tag, dict(attrs).get("id"))

    def handle_endtag(self, tag):
        if tag in ("script", "style"):
            self._in = None

    def handle_data(self, data):
        if self._in is None:
            self.text.append(data)
        elif self._in == ("script", self.script_id):
            self.script += data


def visible_text(page):
    c = _Collect()
    c.feed(page)
    return "".join(c.text)


def script_json(page, script_id):
    c = _Collect(script_id)
    c.feed(page)
    return c.script


class Landing(unittest.TestCase):
    def setUp(self):
        self.page = read("index.html")
        self.home = read("home.html")

    def test_starts_in_mare(self):
        self.assertIn('<body class="mare">', self.page)
        self.assertIn('return "mare";', self.page)  # no saved language or browser language beats the mare start
        self.assertIn(".m{display:none}body.mare .m{display:inline}body.mare .e{display:none}", self.page)

    def test_unmare_button_plain_english(self):
        m = re.search(r'<button class="unmare" id="tg" type="button">([^<]*)</button>', self.page)
        self.assertIsNotNone(m)
        self.assertEqual(m.group(1), "unmare")
        self.assertIn('textContent=c0==="mare"?"unmare":"mare"', self.page)

    def test_switcher_order(self):
        top = self.page[self.page.index('<div class="top">'):self.page.index("</header>")]
        self.assertLess(top.index('id="tg"'), top.index('href="#mare"'))
        order = re.findall(r'<li><a href="#(\w+)" hreflang', top)
        self.assertEqual(order, ["mare"] + LANGS)

    def test_all_five_languages(self):
        self.assertEqual(len(re.findall(r"<article id=", self.page)), 5)
        i18n = json.loads(script_json(self.page, "i18n"))
        self.assertEqual(sorted(i18n), sorted(["mare"] + LANGS))
        for lang in LANGS:
            self.assertIn('<article id="%s"' % lang, self.page)

    def test_real_texts_are_inside(self):
        """The plain texts inside the landing page are the real homepage texts (they may drift: see the pull request)."""
        for lang in LANGS:
            real = re.search(r'<article id="%s".*?</article>' % lang, self.home, re.S).group(0)
            land = re.search(r'<article id="%s".*?</article>' % lang, self.page, re.S).group(0)
            for text in re.findall(r"<(?:p|h2|li)[^>]*>(?:<b>)?([^<]{25,})", real):
                self.assertIn(text.strip(), plain(land), (lang, text[:40]))

    def test_download_and_release_links(self):
        for lang in LANGS:
            a = re.search(r'<article id="%s".*?</article>' % lang, self.page, re.S).group(0)
            self.assertEqual(a.count('class="btn" href="%s"' % WINDOWS_ZIP), 1, lang)
            self.assertEqual(a.count('<a href="%s">' % RELEASE), 1, lang)

    def test_manual_links(self):
        """A link to the manual in the language of the page and a download link for its PDF, in every language, on the
        landing page and the real homepage. English is the fallback and the only one that names no language."""
        for name in ("index.html", "home.html"):
            page = read(name)
            for lang in LANGS:
                a = re.search(r'<article id="%s".*?</article>' % lang, page, re.S).group(0)
                top = re.search(r'<nav class="site".*?</nav>', a, re.S).group(0)
                href = "manual.html" if lang == "en" else "manual.%s.html" % lang
                pdf = "PinglyMotion-Manual.pdf" if lang == "en" else "PinglyMotion-Manual.%s.pdf" % lang
                self.assertEqual(len(re.findall(r'<a href="%s"' % re.escape(href), top)), 1, (name, lang))  # at the top
                self.assertEqual(len(re.findall(r'<a href="manual[.a-z]*html"', a)), 1, (name, lang))
                self.assertEqual(len(re.findall(r'<a href="%s" download>' % re.escape(pdf), a)), 1, (name, lang))
                self.assertEqual(len(re.findall(r'<a href="PinglyMotion-Manual[.a-z]*pdf" download>', a)), 1, (name, lang))
                self.assertNotIn("English", plain(a).replace("Second Life", ""), (name, lang))  # the page names no language
                self.assertNotIn("engelsk", plain(a), (name, lang))
                self.assertNotIn("Englisch", plain(a), (name, lang))
                self.assertNotIn("angielsku", plain(a), (name, lang))
                self.assertNotIn("Engels", plain(a), (name, lang))
                for target in (href, pdf):
                    self.assertTrue(os.path.isfile(os.path.join(DOCS, target)), target)

    def test_mare_manual_link(self):
        """In mare the manual link goes to the mare manual (the landing page script swaps the address)."""
        page = read("index.html")
        en = re.search(r'<article id="en".*?</article>', page, re.S).group(0)
        self.assertEqual(len(re.findall(r'<a href="manual.html" data-manual>', en)), 1)
        self.assertEqual(page.count("data-manual"), 2)  # the link and the script
        self.assertIn('c0==="mare"?"manual.mare.html":"manual.html"', page)
        self.assertTrue(os.path.isfile(os.path.join(DOCS, "manual.mare.html")))

    def test_manual_is_published(self):
        import importlib.util
        spec = importlib.util.spec_from_file_location("bs", os.path.join(HERE, "..", "tools", "site", "build_status.py"))
        b = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(b)
        for name in ("manual.html", "manual.no.html", "manual.de.html", "manual.pl.html", "manual.nl.html", "manual.mare.html",
                     "PinglyMotion-Manual.pdf", "PinglyMotion-Manual.no.pdf", "PinglyMotion-Manual.de.pdf",
                     "PinglyMotion-Manual.pl.pdf", "PinglyMotion-Manual.nl.pdf"):
            self.assertIn(name, b.EXTRA_FILES)
        self.assertIn("cp -r docs/manual _site/manual", read("..", ".github", "workflows", "pages.yml"))

    def test_working_links_kept(self):
        for needle in ('href="download.html#', 'href="privacy.html#', 'href="mockup/"', "CREDITS.md",
                       "issues/new?template=bug_report.yml", "issues/new?template=suggestion.yml"):
            self.assertIn(needle, self.page, needle)
        for name in ("download.html", "privacy.html", "mockup/index.html"):
            self.assertTrue(os.path.isfile(os.path.join(DOCS, name)), name)

    def test_no_external_requests(self):
        self.assertNotIn("<script src=", self.page)
        self.assertNotIn("<link", re.sub(r'<link rel="stylesheet" href="site.css">', "", self.page))
        self.assertNotIn("@import", self.page)
        self.assertNotIn("url(http", self.page)
        for attr in re.findall(r'\b(?:src|action)="([^"]*)"', self.page):
            self.fail("unexpected src/action: " + attr)
        for url in re.findall(r'href="(https?://[^"]+)"', self.page):
            self.assertTrue(url.startswith("https://github.com/pingledingle-tech/PinglyMotion"), url)
        self.assertNotIn("fetch(", self.page)
        self.assertNotIn("XMLHttpRequest", self.page)

    def test_no_personal_data(self):
        self.assertIsNone(re.search(r"[\w.+-]+@[\w-]+\.[\w.]+", self.page))
        self.assertIsNone(re.search(r"C:\\Users|/home/|/Users/", self.page))
        self.assertIsNone(re.search(r"\+?\d[\d ]{8,}\d", visible_text(self.page)))

    def test_real_homepage_kept(self):
        self.assertIn('<meta name="robots" content="noindex">', self.home)
        self.assertNotIn('class="mare"', self.home)
        self.assertIn('<script src="site.js"></script>', self.home)
        self.assertEqual(len(re.findall(r"<article id=", self.home)), 5)
        self.assertNotIn("index.html#", self.home)

    def test_home_in_site_build(self):
        import importlib.util
        spec = importlib.util.spec_from_file_location("bs", os.path.join(HERE, "..", "tools", "site", "build_status.py"))
        b = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(b)
        self.assertIn("home.html", b.EXTRA_FILES)
        self.assertIn("docs/home.html", read("..", ".github", "workflows", "pages.yml"))


if __name__ == "__main__":
    unittest.main()
