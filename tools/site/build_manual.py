#!/usr/bin/env python3
"""Make the manual pages (and optionally the PDFs) from the Markdown manuals in docs/.

Plain standard-library Python. The Markdown reader only knows what the manual uses:
headings, paragraphs, bullet and numbered lists (also nested), tables, images, links,
bold, italic, inline code, code blocks and horizontal rules.

The English docs/MANUAL.md is the one source of truth for the structure. docs/MANUAL.<code>.md
(no, de, pl, nl) are translations with the same chapters and the same images. The "mare" page is
made automatically from the English text: the letters become the small symbol letters of the
mare landing page (docs/index.html), and a short list of words stays readable.

    python3 tools/site/build_manual.py              # writes docs/manual.html and docs/manual.<code>.html
    python3 tools/site/build_manual.py --pdf        # also writes docs/PinglyMotion-Manual[.<code>].pdf
    python3 tools/site/build_manual.py --check      # exit 1 when a page is out of date
    python3 tools/site/build_manual.py --lang no    # only one language (en, no, de, pl, nl, mare)

The PDFs need a headless Chrome or Chromium (or wkhtmltopdf) on the machine. The tests only
check that each PDF file exists and is a valid PDF; they do not make it. There is no PDF for mare.
"""

import html
from html.parser import HTMLParser
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
DOCS = os.path.normpath(os.path.join(HERE, "..", "..", "docs"))
REPO = "https://github.com/pingledingle-tech/PinglyMotion"

# The five site languages, in the order of the homepage switcher. English is the source of truth.
LANGS = ["en", "no", "de", "pl", "nl"]
# Per language: the lang attribute, the name of the language in itself, and the texts around the manual.
LABELS = {
    "en": {"html": "en", "name": "English", "home": "Home", "download": "Download", "pdf": "Manual as PDF",
           "bug": "Report a bug", "skip": "Skip to content", "site": "Site", "langs": "Manual languages",
           "desc": "The PinglyMotion user manual: install, animate, Script Lab, AI assistants, shortcuts and troubleshooting."},
    "no": {"html": "nb", "name": "Norsk", "home": "Hjem", "download": "Last ned", "pdf": "Veiledning som PDF",
           "bug": "Meld en feil", "skip": "Gå til innholdet", "site": "Nettsted", "langs": "Språk for veiledningen",
           "desc": "Brukerveiledning for PinglyMotion: installasjon, animasjon, Script Lab, AI-assistenter, hurtigtaster og feilsøking."},
    "de": {"html": "de", "name": "Deutsch", "home": "Start", "download": "Download", "pdf": "Handbuch als PDF",
           "bug": "Fehler melden", "skip": "Zum Inhalt springen", "site": "Website", "langs": "Sprachen des Handbuchs",
           "desc": "Das PinglyMotion-Handbuch: Installation, Animation, Script Lab, KI-Assistenten, Tastenkürzel und Fehlersuche."},
    "pl": {"html": "pl", "name": "Polski", "home": "Start", "download": "Pobierz", "pdf": "Podręcznik jako PDF",
           "bug": "Zgłoś błąd", "skip": "Przejdź do treści", "site": "Strona", "langs": "Języki podręcznika",
           "desc": "Podręcznik użytkownika PinglyMotion: instalacja, animacja, Script Lab, asystenci AI, skróty klawiszowe i rozwiązywanie problemów."},
    "nl": {"html": "nl", "name": "Nederlands", "home": "Home", "download": "Download", "pdf": "Handleiding als PDF",
           "bug": "Fout melden", "skip": "Naar de inhoud", "site": "Website", "langs": "Talen van de handleiding",
           "desc": "De PinglyMotion-handleiding: installeren, animeren, Script Lab, AI-assistenten, sneltoetsen en problemen oplossen."},
}
MARE = "mare"
PDF_LIMIT = 3 * 1024 * 1024  # a PDF per language is published only when it stays under this size


def source_path(lang):
    return os.path.join(DOCS, "MANUAL.md" if lang == "en" else "MANUAL.%s.md" % lang)


def html_name(lang):
    return "manual.html" if lang == "en" else "manual.%s.html" % lang


def html_path(lang):
    return os.path.join(DOCS, html_name(lang))


def pdf_name(lang):
    return "PinglyMotion-Manual.pdf" if lang == "en" else "PinglyMotion-Manual.%s.pdf" % lang


def pdf_path(lang):
    return os.path.join(DOCS, pdf_name(lang))


# Kept for the older tests and tools.
SOURCE = source_path("en")
TARGET = html_path("en")
PDF = pdf_path("en")

LIST_RE = re.compile(r"^(\s*)([-*]|\d+\.)\s+(.*)$")
HEADING_RE = re.compile(r"^(#{1,4})\s+(.*?)\s*$")
TABLE_SEP_RE = re.compile(r"^\s*\|?\s*:?-{3,}:?\s*(\|\s*:?-{3,}:?\s*)*\|?\s*$")


def slug(text):
    """The anchor of a heading, the way GitHub makes it."""
    text = re.sub(r"<[^>]+>", "", text)
    text = re.sub(r"[*_`]", "", text).lower()
    text = re.sub(r"[^\w\- ]", "", text)
    return text.strip().replace(" ", "-")


def fix_link(url):
    if url.startswith("../"):
        return REPO + "/blob/main/" + url[3:]
    return url


def inline(text):
    codes = []

    def keep_code(m):
        codes.append("<code>" + html.escape(m.group(1), quote=False) + "</code>")
        return "\x00%d\x00" % (len(codes) - 1)

    text = re.sub(r"`([^`]+)`", keep_code, text)
    text = html.escape(text, quote=False)
    text = re.sub(r"!\[([^\]]*)\]\(([^)\s]+)\)",
                  lambda m: '<img src="%s" alt="%s" loading="lazy">' % (html.escape(m.group(2), quote=True), html.escape(m.group(1), quote=True)),
                  text)
    text = re.sub(r"\[([^\]]+)\]\(([^)\s]+)\)",
                  lambda m: '<a href="%s">%s</a>' % (html.escape(fix_link(m.group(2)), quote=True), m.group(1)), text)
    text = re.sub(r"&lt;(https?://[^&\s]+)&gt;", lambda m: '<a href="%s">%s</a>' % (m.group(1), m.group(1)), text)
    text = re.sub(r"\*\*(.+?)\*\*", r"<strong>\1</strong>", text)
    text = re.sub(r"(?<![\w*])\*(?!\s)(.+?)(?<!\s)\*(?![\w*])", r"<em>\1</em>", text)
    return re.sub(r"\x00(\d+)\x00", lambda m: codes[int(m.group(1))], text)


def split_row(line):
    line = line.strip()
    if line.startswith("|"):
        line = line[1:]
    if line.endswith("|"):
        line = line[:-1]
    return [c.strip() for c in re.split(r"(?<!\\)\|", line)]


def parse_blocks(lines):
    out = []
    i = 0
    n = len(lines)
    while i < n:
        line = lines[i]
        if not line.strip():
            i += 1
            continue
        if line.strip().startswith("```"):
            j = i + 1
            body = []
            while j < n and not lines[j].strip().startswith("```"):
                body.append(lines[j])
                j += 1
            out.append("<pre><code>" + html.escape("\n".join(body), quote=False) + "</code></pre>")
            i = j + 1
            continue
        m = HEADING_RE.match(line)
        if m:
            level = len(m.group(1))
            title = m.group(2)
            out.append('<h%d id="%s">%s</h%d>' % (level, slug(title), inline(title), level))
            i += 1
            continue
        if re.match(r"^\s*(---+|\*\*\*+)\s*$", line):
            out.append("<hr>")
            i += 1
            continue
        if line.lstrip().startswith("|") and i + 1 < n and TABLE_SEP_RE.match(lines[i + 1]):
            head = split_row(line)
            j = i + 2
            rows = []
            while j < n and lines[j].lstrip().startswith("|"):
                rows.append(split_row(lines[j]))
                j += 1
            t = ["<table>", "<thead><tr>" + "".join("<th>%s</th>" % inline(c) for c in head) + "</tr></thead>", "<tbody>"]
            for r in rows:
                t.append("<tr>" + "".join("<td>%s</td>" % inline(c) for c in r) + "</tr>")
            t.append("</tbody></table>")
            out.append('<div class="table-wrap">' + "".join(t) + "</div>")
            i = j
            continue
        m = LIST_RE.match(line)
        if m:
            ordered = m.group(2)[0].isdigit()
            base = len(m.group(1))
            items = []
            j = i
            while j < n:
                mm = LIST_RE.match(lines[j])
                if not mm or len(mm.group(1)) != base or mm.group(2)[0].isdigit() != ordered:
                    break
                content_indent = len(mm.group(1)) + len(mm.group(2)) + 1
                body = [mm.group(3)]
                j += 1
                while j < n:
                    nxt = lines[j]
                    if not nxt.strip():
                        k = j
                        while k < n and not lines[k].strip():
                            k += 1
                        if k < n and (len(lines[k]) - len(lines[k].lstrip())) > base and True:
                            body.append("")
                            j += 1
                            continue
                        break
                    indent = len(nxt) - len(nxt.lstrip())
                    if indent > base:
                        body.append(nxt[min(content_indent, indent):] if indent >= content_indent else nxt.lstrip())
                        j += 1
                        continue
                    break
                items.append(body)
                # blank lines between two items of the same list do not end it
                k = j
                while k < n and not lines[k].strip():
                    k += 1
                if k > j and k < n:
                    nm = LIST_RE.match(lines[k])
                    if nm and len(nm.group(1)) == base and nm.group(2)[0].isdigit() == ordered:
                        j = k
            tag = "ol" if ordered else "ul"
            parts = ["<%s>" % tag]
            for body in items:
                inner = parse_blocks(body)
                if len(inner) >= 1 and inner[0].startswith("<p>"):
                    # a tight item: show its first paragraph without the p tags when it is alone
                    if len(inner) == 1:
                        inner[0] = inner[0][3:-4]
                parts.append("<li>%s</li>" % "".join(inner))
            parts.append("</%s>" % tag)
            out.append("".join(parts))
            i = j
            continue
        # a paragraph: until a blank line or the start of another block
        para = [line.strip()]
        j = i + 1
        while j < n and lines[j].strip() and not HEADING_RE.match(lines[j]) and not LIST_RE.match(lines[j]) \
                and not lines[j].lstrip().startswith("|") and not lines[j].strip().startswith("```"):
            para.append(lines[j].strip())
            j += 1
        out.append("<p>%s</p>" % inline(" ".join(para)))
        i = j
    return out


def first_title(md):
    for line in md.splitlines():
        m = HEADING_RE.match(line)
        if m and len(m.group(1)) == 1:
            return m.group(2)
    return "PinglyMotion User Manual"


STYLE = """
.manual{max-width:62rem;margin:0 auto}
.manual img{max-width:100%;height:auto;border:1px solid var(--border,#8884);border-radius:6px;display:block;margin:.75rem 0}
.manual .table-wrap{overflow-x:auto;margin:1rem 0}
.manual table{border-collapse:collapse;width:100%}
.manual th,.manual td{border:1px solid var(--border,#8886);padding:.35rem .6rem;text-align:left;vertical-align:top}
.manual pre{overflow-x:auto;padding:.75rem;border:1px solid var(--border,#8886);border-radius:6px}
.manual h2{margin-top:2.5rem}
@media print{
  html,body{background:#fff !important;color:#111 !important}
  .skip,.top,nav.site,.unmare{display:none}
  .manual img{max-height:22cm;width:auto}
  .manual tr,.manual img{break-inside:avoid}
  .manual h2,.manual h3{break-after:avoid}
  a{color:#0645ad}
}
""".strip()

# Only on the mare page: the same switch as the mare landing page (docs/index.html). The plain English text
# stays in the page for screen readers (visually hidden while mare is on).
MARE_STYLE = """
.m{display:none}body.mare .m{display:inline}
body.mare .e{position:absolute;width:1px;height:1px;overflow:hidden;clip:rect(0 0 0 0);white-space:nowrap}
.aud{font-weight:700}
body{font-family:system-ui,"Segoe UI",Roboto,"DejaVu Sans","Noto Sans","Noto Sans Math","Segoe UI Symbol","Arial Unicode MS",FreeSans,sans-serif}
.unmare{min-height:2.5rem;padding:0 1rem;font:inherit;font-weight:600;cursor:pointer;background:var(--accent,#2b6cb0);color:var(--on-accent,#fff);border:1px solid var(--accent,#2b6cb0);border-radius:6px;margin:.5rem 0}
""".strip()
MARE_SCRIPT = ('(function(){var b=document.getElementById("tg");b.addEventListener("click",function(){'
               'var m=document.body.classList.toggle("mare");b.textContent=m?"unmare":"mare";});})();')

# The mare letters: the same symbol letters as the landing page. Anything that is not a letter stays as it is.
MARE_LETTERS = dict(zip("abcdefghijklmnopqrstuvwxyz", "ᵄᵝᶝᶞᵋᶡᶢʱᶦᶨᶣᶪᶬᶯᵓᶲᵠʵᶳᶵᶶᶹʷᵡʸᶾ"))
# The words that stay readable in mare, as on the landing page.
MARE_READABLE = {"to", "from", "down", "under", "wrong", "silly", "you", "go", "pingle", "pingledingle"}
WORD_RE = re.compile(r"[A-Za-z]+(?:['’][A-Za-z]+)*")


def mare_text(text):
    """Plain text -> HTML of the mare version (readable words get the class aud)."""
    out = []
    pos = 0
    for m in WORD_RE.finditer(text):
        out.append(html.escape(text[pos:m.start()], quote=False))
        word = m.group(0)
        if word.lower() in MARE_READABLE:
            out.append('<span class="aud">%s</span>' % word)
        else:
            out.append("".join(MARE_LETTERS.get(c.lower(), c) for c in word))
        pos = m.end()
    out.append(html.escape(text[pos:], quote=False))
    return "".join(out)


class _MareMaker(HTMLParser):
    """Copies a piece of HTML and wraps every text node in a mare span and a plain English span.
    Code, code blocks and web addresses that are shown as the link text stay as they are."""

    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.out = []
        self.skip = 0
        self.href = None

    def handle_starttag(self, tag, attrs):
        self.out.append(self.get_starttag_text())
        if tag in ("code", "pre"):
            self.skip += 1
        if tag == "a":
            self.href = dict(attrs).get("href")

    def handle_endtag(self, tag):
        self.out.append("</%s>" % tag)
        if tag in ("code", "pre"):
            self.skip -= 1
        if tag == "a":
            self.href = None

    def handle_data(self, data):
        plain = html.escape(data, quote=False)
        if self.skip or not data.strip() or (self.href and data.strip() == self.href):
            self.out.append(plain)
            return
        self.out.append('<span class="m" aria-hidden="true">%s</span><span class="e">%s</span>' % (mare_text(data), plain))


def to_mare(fragment):
    maker = _MareMaker()
    maker.feed(fragment)
    maker.close()
    return "".join(maker.out)


def language_nav(current):
    """The links to the manual in every language, the current one marked."""
    items = []
    for code in LANGS + [MARE]:
        name = "Mare" if code == MARE else LABELS[code]["name"]
        href = "manual.mare.html" if code == MARE else html_name(code)
        lang = "en" if code == MARE else LABELS[code]["html"]
        mark = ' aria-current="page"' if code == current else ""
        items.append('<li><a href="%s" hreflang="%s" lang="%s"%s>%s</a></li>' % (href, lang, lang, mark, name))
    return "<ul>" + "".join(items) + "</ul>"


def build_html(md, lang="en"):
    labels = LABELS[lang]
    body = "\n".join(parse_blocks(md.splitlines()))
    title = html.escape(first_title(md), quote=False)
    pdf = pdf_name(lang) if lang in PDF_LANGS else pdf_name("en")
    return """<!DOCTYPE html>
<html lang="%(lang)s" data-theme="light">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="color-scheme" content="light dark">
<meta name="referrer" content="no-referrer">
<title>%(title)s</title>
<meta name="description" content="%(desc)s">
<link rel="stylesheet" href="site.css">
<style>
%(style)s
</style>
</head>
<body>
<a class="skip" href="#manual">%(skip)s</a>
<div class="wrap">
<nav class="site" aria-label="%(site)s"><ul><li><a href="index.html#%(code)s">%(home)s</a></li><li><a href="download.html#%(code)s">%(download)s</a></li><li><a href="%(pdf)s">%(pdflabel)s</a></li><li><a href="%(repo)s/issues">%(bug)s</a></li></ul></nav>
<nav class="site" aria-label="%(langs)s">%(langnav)s</nav>
<main class="manual" id="manual" lang="%(lang)s">
<!-- Made by tools/site/build_manual.py from docs/%(source)s. Do not edit by hand. -->
%(body)s
</main>
</div>
</body>
</html>
""" % {"lang": labels["html"], "code": lang, "title": title, "style": STYLE, "body": body, "repo": REPO,
       "desc": html.escape(labels["desc"], quote=True), "skip": labels["skip"], "site": labels["site"],
       "home": labels["home"], "download": labels["download"], "bug": labels["bug"], "langs": labels["langs"],
       "pdf": pdf, "pdflabel": labels["pdf"], "langnav": language_nav(lang), "source": os.path.basename(source_path(lang))}


MARE_NOTE = ("This is the mare version of the English manual. A program made it from the English text: "
             "the letters are swapped for small symbols, and a few words stay readable. "
             "Press unmare for plain English.")


def build_mare_html(md):
    """The mare page: made from the English text only, with the same blocks and the same images."""
    body = "\n".join(parse_blocks(md.splitlines()))
    nav = ('<nav class="site" aria-label="Site"><ul><li><a href="index.html#mare">Home</a></li>'
           '<li><a href="manual.html">Manual (English)</a></li><li><a href="download.html#mare">Download</a></li></ul></nav>')
    nav = to_mare(nav)
    title = html.escape(first_title(md), quote=False)
    return """<!DOCTYPE html>
<html lang="en" data-theme="light">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="color-scheme" content="light dark">
<meta name="referrer" content="no-referrer">
<title>%(mtitle)s</title>
<meta name="description" content="%(desc)s">
<link rel="stylesheet" href="site.css">
<style>
%(style)s
%(mstyle)s
</style>
</head>
<body class="mare">
<a class="skip" href="#manual"><span class="m" aria-hidden="true">%(skip)s</span><span class="e">Skip to content</span></a>
<div class="wrap">
<button class="unmare" id="tg" type="button">unmare</button>
%(nav)s
<nav class="site" aria-label="Manual languages">%(langnav)s</nav>
<main class="manual" id="manual" lang="en">
<!-- Made by tools/site/build_manual.py from docs/MANUAL.md. Do not edit by hand. -->
%(note)s
%(body)s
</main>
</div>
<script>%(script)s</script>
</body>
</html>
""" % {"title": title, "mtitle": re.sub(r"<[^>]+>", "", mare_text(first_title(md))),
       "desc": html.escape(LABELS["en"]["desc"], quote=True), "style": STYLE, "mstyle": MARE_STYLE, "nav": nav,
       "skip": mare_text("Skip to content"), "langnav": language_nav(MARE), "script": MARE_SCRIPT,
       "note": to_mare("<p>%s</p>" % html.escape(MARE_NOTE, quote=False)),
       "body": to_mare(body)}


# Languages that get a PDF (each must stay under PDF_LIMIT; set after the PDFs were made).
PDF_LANGS = ["en", "no", "de", "pl", "nl"]


def read_source(lang="en"):
    with open(source_path(lang), encoding="utf-8") as f:
        return f.read()


def page_text(code):
    """The HTML text of the page for a language code or "mare"."""
    if code == MARE:
        return build_mare_html(read_source("en"))
    return build_html(read_source(code), code)


def target_path(code):
    return os.path.join(DOCS, "manual.mare.html") if code == MARE else html_path(code)


def write_html(code="en"):
    out = page_text(code)
    with open(target_path(code), "w", encoding="utf-8", newline="\n") as f:
        f.write(out)
    return out


def find_browser():
    for name in ("google-chrome", "google-chrome-stable", "chromium", "chromium-browser"):
        path = shutil.which(name)
        if path:
            return ("chrome", path)
    path = shutil.which("wkhtmltopdf")
    if path:
        return ("wkhtmltopdf", path)
    return None


def write_pdf(lang="en"):
    found = find_browser()
    if not found:
        raise SystemExit("No headless Chrome, Chromium or wkhtmltopdf found; cannot make the PDF.")
    kind, path = found
    source = html_path(lang)
    with tempfile.TemporaryDirectory() as tmp:
        raw = os.path.join(tmp, "raw.pdf")
        if kind == "chrome":
            cmd = [path, "--headless", "--no-sandbox", "--disable-gpu", "--no-pdf-header-footer",
                   "--print-to-pdf=" + raw, "file://" + source]
        else:
            cmd = [path, "--enable-local-file-access", "--print-media-type", source, raw]
        subprocess.run(cmd, check=True, timeout=300, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        shutil.copyfile(raw, pdf_path(lang))
    return os.path.getsize(pdf_path(lang))


def main(argv):
    codes = LANGS + [MARE]
    if "--lang" in argv:
        codes = [argv[argv.index("--lang") + 1]]
    if "--check" in argv:
        bad = 0
        for code in codes:
            want = page_text(code)
            try:
                have = open(target_path(code), encoding="utf-8").read()
            except OSError:
                have = ""
            if have != want:
                print("docs/%s is out of date: run python3 tools/site/build_manual.py" % os.path.basename(target_path(code)))
                bad = 1
        if not bad:
            print("the manual pages are up to date")
        return bad
    for code in codes:
        write_html(code)
        print("wrote", target_path(code))
    if "--pdf" in argv:
        for code in codes:
            if code == MARE:
                continue
            size = write_pdf(code)
            print("wrote %s (%d bytes)" % (pdf_path(code), size))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
