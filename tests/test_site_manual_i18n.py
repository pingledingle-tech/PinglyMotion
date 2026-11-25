#!/usr/bin/env python3
"""Tests for the translated manuals (Norwegian, German, Polish, Dutch) and the mare manual.

The English docs/MANUAL.md is the one source of truth for the structure. A translation has the same
chapters, the same headings (same levels, same order), the same tables, the same images (shared, not
copied), the same code, file names and menu names. The mare page is made from the English text by
tools/site/build_manual.py. Every page links to the other languages, and every link resolves.

Run: python3 tests/test_site_manual_i18n.py
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

OTHER = ["no", "de", "pl", "nl"]
IMAGE_RE = re.compile(r"!\[([^\]]*)\]\(([^)\s]+)\)")
NOTE_RE = re.compile(r"^\*[^*].*\*$")
PDF_LIMIT = 3 * 1024 * 1024


def read(path):
    with open(path, encoding="utf-8") as f:
        return f.read()


def headings(text):
    return [(len(m.group(1)), m.group(2)) for m in (bm.HEADING_RE.match(l) for l in text.splitlines()) if m]


def shape(text):
    """The structure of a manual line by line: headings, blank lines, table rows, list items, images.
    The one translation note (an italic line before the contents) is left out, with one blank line after it."""
    lines = text.splitlines()
    out = []
    skip_blank = False
    for i, line in enumerate(lines):
        if i < 12 and NOTE_RE.match(line) and not line.startswith("*Diagram"):
            skip_blank = True
            continue
        if skip_blank and not line.strip():
            skip_blank = False
            continue
        skip_blank = False
        m = bm.HEADING_RE.match(line)
        if m:
            out.append(("h", len(m.group(1))))
        elif not line.strip():
            out.append(("blank",))
        else:
            kind = ""
            if line.startswith("|"):
                kind += "|%d" % line.count("|")
            if "![" in line:
                kind += "img"
            lm = bm.LIST_RE.match(line)
            if lm:
                kind += "list%d%s" % (len(lm.group(1)), lm.group(2)[0].isdigit())
            out.append(("line", kind, line.count("](#")))
    return out


class Translations(unittest.TestCase):
    def setUp(self):
        self.en = read(bm.source_path("en"))

    def test_files_exist(self):
        for lang in OTHER:
            self.assertTrue(os.path.isfile(bm.source_path(lang)), lang)
            self.assertTrue(os.path.isfile(bm.html_path(lang)), lang)

    def test_same_headings_as_english(self):
        en = headings(self.en)
        self.assertGreaterEqual(len(en), 60)
        for lang in OTHER:
            text = read(bm.source_path(lang))
            got = headings(text)
            self.assertEqual([lv for lv, _ in got], [lv for lv, _ in en], lang)
            self.assertEqual(len({t for _, t in got}), len(got), "two equal headings in " + lang)
            # the chapters are numbered the same way
            self.assertEqual([bool(re.search(r"\d+", t)) for _, t in got], [bool(re.search(r"\d+", t)) for _, t in en], lang)

    def test_same_structure_as_english(self):
        want = shape(self.en)
        for lang in OTHER:
            self.assertEqual(shape(read(bm.source_path(lang))), want, lang)

    def test_same_images_in_the_same_order(self):
        want = [p for _, p in IMAGE_RE.findall(self.en)]
        self.assertGreaterEqual(len(want), 20)
        for lang in OTHER:
            text = read(bm.source_path(lang))
            found = IMAGE_RE.findall(text)
            self.assertEqual([p for _, p in found], want, lang)
            for alt, path in found:
                self.assertGreater(len(alt.split()), 2, "alt text too short: %s (%s)" % (path, lang))
                self.assertTrue(os.path.isfile(os.path.join(DOCS, path)), path)
                self.assertTrue(path.startswith("manual/img/"), path)  # shared, never copied

    def test_images_are_not_copied(self):
        for name in os.listdir(DOCS):
            if name.startswith("manual") and os.path.isdir(os.path.join(DOCS, name)):
                self.assertEqual(name, "manual", "an image folder per language: " + name)
        for _, _, files in os.walk(os.path.join(DOCS, "manual")):
            self.assertTrue(all(f.endswith(".png") for f in files))
        self.assertEqual(sorted(os.listdir(os.path.join(DOCS, "manual"))), ["img"])

    def test_code_and_names_stay_as_they_are(self):
        want = sorted(re.findall(r"`([^`]+)`", self.en))
        for lang in OTHER:
            self.assertEqual(sorted(re.findall(r"`([^`]+)`", read(bm.source_path(lang)))), want, lang)

    def test_menu_names_stay_as_they_are(self):
        # the program has no translation yet: every bold program word of the English manual is in the translation
        bold = set(re.findall(r"\*\*([A-Z][A-Za-z ]*(?: > [A-Z][A-Za-z .]*)+)\*\*", self.en))
        self.assertGreater(len(bold), 20)
        for lang in OTHER:
            text = read(bm.source_path(lang))
            for name in bold:
                self.assertIn("**%s**" % name, text, (lang, name))

    def test_internal_links_resolve(self):
        for lang in OTHER:
            text = read(bm.source_path(lang))
            anchors = {bm.slug(t) for _, t in headings(text)}
            links = re.findall(r"\]\(#([^)]+)\)", text)
            self.assertEqual(len(links), len(re.findall(r"\]\(#([^)]+)\)", self.en)), lang)
            for target in links:
                self.assertIn(target, anchors, "broken link #%s in %s" % (target, lang))

    def test_note_about_machine_translation(self):
        for lang in OTHER:
            lines = read(bm.source_path(lang)).splitlines()
            notes = [l for l in lines[:12] if NOTE_RE.match(l) and not l.startswith("*Diagram")]
            self.assertEqual(len(notes), 1, lang)
            self.assertIn("(manual.html)", notes[0], lang)  # points to the English manual

    def test_no_personal_data(self):
        for lang in OTHER:
            text = read(bm.source_path(lang))
            self.assertIsNone(re.search(r"[\w.+-]+@[\w-]+\.[\w.-]+", text), "an email address in " + lang)
            self.assertIsNone(re.search(r"[A-Za-z]:\\Users\\(?!<)", text), lang)
            self.assertIsNone(re.search(r"/(home|Users)/(?!<)[\w.-]+", text), lang)
            self.assertNotIn("Co-Authored-By", text)


class Pages(unittest.TestCase):
    def test_pages_are_up_to_date(self):
        for code in bm.LANGS + [bm.MARE]:
            self.assertEqual(read(bm.target_path(code)), bm.page_text(code),
                             "docs/%s is out of date: run python3 tools/site/build_manual.py" % os.path.basename(bm.target_path(code)))

    def test_pages_use_the_shared_images(self):
        want = [p for _, p in IMAGE_RE.findall(read(bm.source_path("en")))]
        for code in bm.LANGS + [bm.MARE]:
            page = read(bm.target_path(code))
            srcs = re.findall(r'<img src="([^"]+)"', page)
            self.assertEqual(srcs, want, code)
            for tag in re.findall(r"<img [^>]*>", page):
                self.assertRegex(tag, r'alt="[^"]{5,}"')
            for src in srcs:
                self.assertTrue(os.path.isfile(os.path.join(DOCS, src)), src)

    def test_pages_have_the_same_headings_as_english(self):
        want = [(int(t[1]),) for t in re.findall(r"<(h[1-4]) id=", read(bm.html_path("en")))]
        for code in bm.LANGS + [bm.MARE]:
            page = read(bm.target_path(code))
            got = [(int(t[1]),) for t in re.findall(r"<(h[1-4]) id=", page)]
            self.assertEqual(got, want, code)
            self.assertEqual(len(re.findall(r"<table>", page)), len(re.findall(r"<table>", read(bm.html_path("en")))), code)

    def test_languages_and_titles(self):
        for code in bm.LANGS:
            page = read(bm.html_path(code))
            self.assertIn('<html lang="%s"' % bm.LABELS[code]["html"], page)
            self.assertIn('<main class="manual" id="manual" lang="%s">' % bm.LABELS[code]["html"], page)
            self.assertNotIn("<script", page)
        titles = {re.search(r"<title>(.*?)</title>", read(bm.html_path(c))).group(1) for c in bm.LANGS}
        self.assertEqual(len(titles), 5)

    def test_links_resolve(self):
        for code in bm.LANGS + [bm.MARE]:
            page = read(bm.target_path(code))
            ids = set(re.findall(r'id="([^"]+)"', page))
            for href in re.findall(r'<a [^>]*href="([^"]+)"', page):
                if href.startswith("#"):
                    self.assertIn(href[1:], ids, (code, href))
                elif href.startswith(("https://", "mailto:")):
                    continue
                else:
                    target = href.split("#")[0]
                    self.assertTrue(os.path.isfile(os.path.join(DOCS, target)), (code, href))

    def test_every_page_links_to_every_language(self):
        for code in bm.LANGS + [bm.MARE]:
            page = read(bm.target_path(code))
            nav = re.search(r'<nav class="site" aria-label="[^"]*">(<ul><li><a href="manual[^>]*>.*?)</nav>', page, re.S).group(1)
            hrefs = re.findall(r'<a href="([^"]+)"', nav)
            self.assertEqual(hrefs, ["manual.html", "manual.no.html", "manual.de.html", "manual.pl.html", "manual.nl.html",
                                     "manual.mare.html"], code)
            self.assertEqual(nav.count('aria-current="page"'), 1, code)

    def test_home_and_download_links_use_the_language(self):
        for code in bm.LANGS:
            page = read(bm.html_path(code))
            self.assertIn('<a href="index.html#%s">' % code, page)
            self.assertIn('<a href="download.html#%s">' % code, page)

    def test_pdf_links(self):
        for code in bm.LANGS:
            page = read(bm.html_path(code))
            name = bm.pdf_name(code) if code in bm.PDF_LANGS else bm.pdf_name("en")
            self.assertIn('<a href="%s">' % name, page)


class Pdfs(unittest.TestCase):
    def test_a_pdf_per_language_under_3_mb(self):
        self.assertIn("en", bm.PDF_LANGS)
        for code in bm.LANGS:
            path = bm.pdf_path(code)
            if code not in bm.PDF_LANGS:
                self.assertFalse(os.path.exists(path), "a PDF that is not published: " + path)
                continue
            self.assertTrue(os.path.isfile(path), path)
            size = os.path.getsize(path)
            self.assertGreater(size, 50 * 1024)
            self.assertLess(size, PDF_LIMIT, "%s is over 3 MB" % path)
            with open(path, "rb") as f:
                data = f.read()
            self.assertTrue(data.startswith(b"%PDF-"))
            self.assertIn(b"%%EOF", data[-2048:])
            self.assertGreaterEqual(len(re.findall(rb"/Type\s*/Page[^s]", data)), 10)
            self.assertIsNone(re.search(rb"/(home|Users|tmp)/[\w.-]+", data), "a folder path inside " + path)

    def test_no_pdf_for_mare(self):
        self.assertFalse(os.path.exists(os.path.join(DOCS, "PinglyMotion-Manual.mare.pdf")))


class Mare(unittest.TestCase):
    def setUp(self):
        self.page = read(bm.target_path(bm.MARE))

    def test_same_letters_as_the_landing_page(self):
        # the landing page (docs/index.html) has these mare texts; the manual makes the same letters
        landing = read(os.path.join(DOCS, "index.html"))
        for plain, english in (("Download the early preview", True), ("What it does.", True), ("Release notes and all files", True),
                               ("Skip to content", True), ("Try the clickable mockup", True)):
            mare = re.sub(r"<[^>]+>", "", bm.mare_text(plain, english))
            self.assertIn(mare, re.sub(r"<[^>]+>", "", landing), plain)

    def test_other_languages_use_the_same_letters_too(self):
        landing = read(os.path.join(DOCS, "index.html"))
        stripped = re.sub(r"<[^>]+>", "", landing)
        for plain in ("Gå til innholdet", "Przejdź do treści", "Zum Inhalt springen", "Naar de inhoud"):
            self.assertIn(re.sub(r"<[^>]+>", "", bm.mare_text(plain, False)), stripped, plain)

    def test_readable_words_are_the_landing_page_words(self):
        landing = read(os.path.join(DOCS, "index.html"))
        used = {w.lower() for w in re.findall(r'<span class="aud">(.*?)</span>', landing)}
        self.assertTrue(used <= bm.MARE_READABLE, used - bm.MARE_READABLE)
        self.assertIn('<span class="aud">to</span>', bm.mare_text("how to start"))
        self.assertNotIn('class="aud"', bm.mare_text("hello world"))

    def test_symbols_and_plain_twin(self):
        self.assertIn('<body class="mare">', self.page)
        self.assertIn('id="tg"', self.page)
        self.assertIn(">unmare</button>", self.page)
        self.assertIn(".m{display:none}body.mare .m{display:inline}", self.page)
        spans = re.findall(r'<span class="m" aria-hidden="true">(.*?)</span><span class="e">(.*?)</span>', self.page, re.S)
        self.assertGreater(len(spans), 500)
        joined = " ".join(p for _, p in spans)
        self.assertIn("Your first animation", joined)
        self.assertIn("Every menu item", joined)

    def test_code_and_addresses_stay_readable(self):
        self.assertIn("<code>PinglyMotion.exe</code>", self.page)
        self.assertIn("<code>-60</code>", self.page)
        self.assertIn('<a href="https://github.com/pingledingle-tech/PinglyMotion/releases">https://github.com/pingledingle-tech/PinglyMotion/releases</a>', self.page)

    def test_every_letter_of_prose_is_a_symbol(self):
        # outside code, addresses and the plain twins, no ordinary Latin letters stay (except the readable words)
        body = self.page.split("Do not edit by hand. -->", 1)[1]
        body = re.sub(r"<pre>.*?</pre>", "", body, flags=re.S)
        body = re.sub(r"<code>.*?</code>", "", body, flags=re.S)
        body = re.sub(r'<span class="e">.*?</span>', "", body, flags=re.S)
        body = re.sub(r'<span class="aud">.*?</span>', "", body, flags=re.S)
        body = re.sub(r'<a href="https://[^"]*">https://[^<]*</a>', "", body)
        body = body.split("</main>", 1)[0]  # the script comes after the text
        text = re.sub(r"<[^>]+>", "", body)
        self.assertIsNone(re.search(r"[A-Za-z]{3}", text), re.search(r".{20}[A-Za-z]{3}.{20}", text))

    def test_note_says_what_it_is(self):
        self.assertIn("mare version", self.page)
        self.assertIn("Manual (English)", self.page)  # a link to the plain English manual

    def test_script_only_toggles(self):
        self.assertEqual(self.page.count("<script"), 1)
        start = self.page.index("<script>") + len("<script>")
        self.assertEqual(self.page[start:self.page.index("</script>", start)], bm.MARE_SCRIPT)
        self.assertNotIn("http", bm.MARE_SCRIPT)
        self.assertNotIn("fetch", bm.MARE_SCRIPT)

    def test_page_size_is_sane(self):
        self.assertLess(os.path.getsize(bm.target_path(bm.MARE)), 700 * 1024)
        for code in bm.LANGS:
            self.assertLess(os.path.getsize(bm.html_path(code)), 200 * 1024, code)


class Builder(unittest.TestCase):
    def test_mare_converter_keeps_markup(self):
        out = bm.to_mare('<p>Press <strong>Start</strong> &amp; <code>x</code> to go. <a href="https://e.org/a">https://e.org/a</a></p>')
        self.assertIn("<strong>", out)
        self.assertIn("<code>x</code>", out)
        self.assertIn('<span class="aud">to</span>', out)
        self.assertIn('<a href="https://e.org/a">https://e.org/a</a>', out)
        self.assertIn('<span class="e">Press </span>', out)
        self.assertIn("&amp;", out)

    def test_language_names(self):
        self.assertEqual(bm.LANGS, ["en", "no", "de", "pl", "nl"])  # the five site languages, as the homepage has them
        home = read(os.path.join(DOCS, "home.html"))
        self.assertEqual(re.findall(r'<article id="(\w+)"', home), bm.LANGS)

    def test_file_names(self):
        self.assertEqual(bm.html_name("en"), "manual.html")
        self.assertEqual(bm.html_name("no"), "manual.no.html")
        self.assertEqual(bm.pdf_name("de"), "PinglyMotion-Manual.de.pdf")
        self.assertEqual(os.path.basename(bm.source_path("pl")), "MANUAL.pl.md")

    def test_check_command(self):
        self.assertEqual(bm.main(["--check"]), 0)


if __name__ == "__main__":
    unittest.main()
