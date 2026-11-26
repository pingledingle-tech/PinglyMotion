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
WINDOWS_ZIP = "https://github.com/pingledingle-tech/PinglyMotion/releases/download/v0.3.0-beta.1/PinglyMotion-v0.3.0-beta.1-windows.zip"
LINUX_APPIMAGE = "https://github.com/pingledingle-tech/PinglyMotion/releases/download/v0.3.0-beta.1/PinglyMotion-v0.3.0-beta.1-linux-x86_64.AppImage"


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
        page = read("home.html")
        for lang in LANGS:
            a = article(page, lang)
            btn = re.search(r'<a class="btn" href="%s">[^<]+</a>' % re.escape(WINDOWS_ZIP), a)
            self.assertIsNotNone(btn, lang)
            self.assertLess(a.index('class="tagline'), btn.start(), lang)
            self.assertLess(btn.start(), a.index("<h2"), lang)

    def test_release_notes_link_separate(self):
        for name in ("home.html", "download.html"):
            page = read(name)
            for lang in LANGS:
                a = article(page, lang)
                self.assertEqual(a.count('<a href="%s">' % RELEASE), 1, (name, lang))
                self.assertNotIn('class="btn" href="%s"' % RELEASE, a, (name, lang))

    def test_download_page_direct_links(self):
        page = read("download.html")
        for lang in LANGS:
            a = article(page, lang)
            self.assertEqual(a.count('<a class="btn" href="%s">' % WINDOWS_ZIP), 1, lang)
            self.assertEqual(a.count('<a href="%s">' % LINUX_APPIMAGE), 1, lang)

    def test_short(self):
        page = read("home.html")
        for lang in LANGS:
            # 400 until the owner-approved status box and licence text (text corrections C-02, C-04) were added
            self.assertLess(words(article(page, lang)), 500, lang)

    def test_english_and_norwegian_text(self):
        page = read("home.html")
        en, no = article(page, "en"), article(page, "no")
        for needle in ("What it does.", "No AI was harmed in the making of this tool.", "a lot of Lindens", "Pingle is a she"):
            self.assertIn(needle, en)
        for needle in ("Hva den gjør.", "Pingle er en hun", "mange Lindens"):
            self.assertIn(needle, no)

    def test_same_structure_in_every_language(self):
        page = read("home.html")
        for lang in LANGS:
            a = article(page, lang)
            self.assertEqual(a.count("<h2"), 3, lang)
            self.assertEqual(a.count("<section"), 4, lang)
            self.assertEqual(a.count('class="btn"'), 2, lang)
            self.assertEqual(a.count("<b>"), 3, lang)
            self.assertIn("Pingle", a, lang)
            for k in ("-h2", "-h3", "-hdl"):
                self.assertIn('id="%s%s"' % (lang, k), a, lang)

    def test_experienced_users_sentence_in_every_language(self):
        page = read("home.html")
        want = {
            "en": "And experienced animators and scripters who want a free tool they can bend to their own way of working.",
            "no": "Og erfarne animatører og skriptere som vil ha et gratis verktøy de kan forme etter sin egen måte å jobbe på.",
            "de": "Und erfahrene Animatoren und Skripter, die ein kostenloses Werkzeug wollen, das sie an ihre eigene Arbeitsweise anpassen können.",
            "pl": "Oraz doświadczeni animatorzy i skrypterzy, którzy chcą darmowego narzędzia, które mogą dopasować do własnego sposobu pracy.",
            "nl": "En ervaren animators en scripters die een gratis hulpmiddel willen dat ze kunnen aanpassen aan hun eigen manier van werken.",
        }
        for lang, text in want.items():
            sec = re.search(r'<section aria-labelledby="%s-h3">.*?</section>' % lang, article(page, lang), re.S).group(0)
            self.assertTrue(re.search(re.escape(text) + r"</p>", sec), lang)

    def test_mockup_note_text(self):
        page = read("home.html")
        want = {
            "en": "The mockup is just a web page and doesn't quite show how this really works.",
            "no": "Mockupen er bare en nettside, og viser ikke helt hvordan dette egentlig virker.",
            "de": "Der Mockup ist nur eine Webseite und zeigt nicht ganz, wie das in Wirklichkeit funktioniert.",
            "pl": "Makieta to tylko strona internetowa i nie pokazuje do końca, jak to naprawdę działa.",
            "nl": "De mockup is maar een webpagina en laat niet helemaal zien hoe dit echt werkt.",
        }
        for lang, text in want.items():
            self.assertIn("<p>%s</p></aside>" % text, article(page, lang), lang)
        for old in ("design direction", "designretningen", "Designrichtung", "kierunek projektu", "ontwerprichting"):
            self.assertNotIn(old, page, old)
        self.assertIn(want["en"], read("mockup/index.html"))

    def test_no_old_wording(self):
        for name in ("home.html", "download.html"):
            page = read(name)
            for old in ("is on its way", "er på vei", "still being prepared", "noch vorbereitet", "jeszcze przygotowywane", "nog voorbereid", "fortsatt under arbeid"):
                self.assertNotIn(old, page, (name, old))

    def test_download_page_links_release(self):
        page = read("download.html")
        for lang in LANGS:
            self.assertIn(RELEASE, article(page, lang), lang)

    def test_headings_in_order(self):
        page = read("home.html")
        for lang in LANGS:
            a = article(page, lang)
            self.assertNotIn("<h1", a)
            self.assertNotIn("<h3", a)
            self.assertGreaterEqual(a.count("<h2"), 1, lang)


if __name__ == "__main__":
    unittest.main()
