#!/usr/bin/env python3
"""Make docs/manual.html (and optionally docs/PinglyMotion-Manual.pdf) from docs/MANUAL.md.

Plain standard-library Python. The Markdown reader only knows what the manual uses:
headings, paragraphs, bullet and numbered lists (also nested), tables, images, links,
bold, italic, inline code, code blocks and horizontal rules.

    python3 tools/site/build_manual.py          # writes docs/manual.html
    python3 tools/site/build_manual.py --pdf    # also writes docs/PinglyMotion-Manual.pdf
    python3 tools/site/build_manual.py --check  # exit 1 when docs/manual.html is out of date

The PDF needs a headless Chrome or Chromium (or wkhtmltopdf) on the machine. The test only
checks that the PDF file exists and is a valid PDF; it does not make it.
"""

import html
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
DOCS = os.path.normpath(os.path.join(HERE, "..", "..", "docs"))
SOURCE = os.path.join(DOCS, "MANUAL.md")
TARGET = os.path.join(DOCS, "manual.html")
PDF = os.path.join(DOCS, "PinglyMotion-Manual.pdf")
REPO = "https://github.com/pingledingle-tech/PinglyMotion"

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
  .skip,.top,nav.site{display:none}
  .manual img{max-height:22cm;width:auto}
  .manual tr,.manual img{break-inside:avoid}
  .manual h2,.manual h3{break-after:avoid}
  a{color:#0645ad}
}
""".strip()


def build_html(md):
    body = "\n".join(parse_blocks(md.splitlines()))
    title = html.escape(first_title(md), quote=False)
    return """<!DOCTYPE html>
<html lang="en" data-theme="light">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="color-scheme" content="light dark">
<meta name="referrer" content="no-referrer">
<title>%(title)s</title>
<meta name="description" content="The PinglyMotion user manual: install, animate, Script Lab, AI assistants, shortcuts and troubleshooting.">
<link rel="stylesheet" href="site.css">
<style>
%(style)s
</style>
</head>
<body>
<a class="skip" href="#manual">Skip to content</a>
<div class="wrap">
<nav class="site" aria-label="Site"><ul><li><a href="index.html#en">Home</a></li><li><a href="download.html#en">Download</a></li><li><a href="PinglyMotion-Manual.pdf">Manual as PDF</a></li><li><a href="%(repo)s/issues">Report a bug</a></li></ul></nav>
<main class="manual" id="manual" lang="en">
<!-- Made by tools/site/build_manual.py from docs/MANUAL.md. Do not edit by hand. -->
%(body)s
</main>
</div>
</body>
</html>
""" % {"title": title, "style": STYLE, "body": body, "repo": REPO}


def read_source():
    with open(SOURCE, encoding="utf-8") as f:
        return f.read()


def write_html():
    out = build_html(read_source())
    with open(TARGET, "w", encoding="utf-8", newline="\n") as f:
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


def write_pdf():
    found = find_browser()
    if not found:
        raise SystemExit("No headless Chrome, Chromium or wkhtmltopdf found; cannot make the PDF.")
    kind, path = found
    with tempfile.TemporaryDirectory() as tmp:
        raw = os.path.join(tmp, "raw.pdf")
        if kind == "chrome":
            cmd = [path, "--headless", "--no-sandbox", "--disable-gpu", "--no-pdf-header-footer",
                   "--print-to-pdf=" + raw, "file://" + TARGET]
        else:
            cmd = [path, "--enable-local-file-access", "--print-media-type", TARGET, raw]
        subprocess.run(cmd, check=True, timeout=300, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        shutil.copyfile(raw, PDF)
    return os.path.getsize(PDF)


def main(argv):
    if "--check" in argv:
        want = build_html(read_source())
        try:
            have = open(TARGET, encoding="utf-8").read()
        except OSError:
            have = ""
        if have != want:
            print("docs/manual.html is out of date: run python3 tools/site/build_manual.py")
            return 1
        print("docs/manual.html is up to date")
        return 0
    write_html()
    print("wrote", TARGET)
    if "--pdf" in argv:
        size = write_pdf()
        print("wrote %s (%d bytes)" % (PDF, size))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
