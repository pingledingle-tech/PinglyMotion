#!/usr/bin/env python3
"""List the user-facing strings of the program (issue 54, teaching texts plan).

Usage: python3 tools/extract_texts.py [--markdown] [--root DIR]

Reads src/*.cpp, src/*.h and src/*.ui and prints one line per string:
file:line, kind, text. Kinds: tr (tr("...") call), ui (<string> in a .ui
file), tip (setToolTip / setStatusTip / setWhatsThis), msg (QMessageBox,
statusBar messages). With --markdown it prints a table for
docs/TEACHING_TEXTS.md. Standard library only.
"""

import argparse
import glob
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
STR = r'"((?:[^"\\]|\\.)*)"'
TR_RE = re.compile(r'(?:QCoreApplication::translate\([^,]+,\s*|\btr\(\s*)' + STR)
TIP_RE = re.compile(r'set(?:ToolTip|StatusTip|WhatsThis)\(\s*' + STR)
MSG_RE = re.compile(r'(?:QMessageBox::\w+|showMessage)\(.*?' + STR)
UI_RE = re.compile(r'<string(?: [^>]*)?>(.*?)</string>')
PROP_RE = re.compile(r'<property name="(\w+)"')
UI_PROPS = ('text', 'title', 'windowTitle', 'toolTip', 'statusTip', 'whatsThis',
            'placeholderText', 'suffix', 'prefix')


def unescape(s):
    return s.replace('\\n', ' / ').replace('\\"', '"').replace('\\\\', '\\')


def scan(root):
    rows = []
    for path in sorted(glob.glob(os.path.join(root, 'src', '*'))):
        ext = os.path.splitext(path)[1]
        rel = os.path.relpath(path, root)
        if ext not in ('.cpp', '.h', '.ui'):
            continue
        prop = ''
        with open(path, encoding='utf-8', errors='replace') as f:
            for n, line in enumerate(f, 1):
                if ext == '.ui':
                    pm = PROP_RE.search(line)
                    if pm:
                        prop = pm.group(1)
                    m = UI_RE.search(line)
                    if m and prop in UI_PROPS:
                        t = m.group(1).strip()
                        t = t.replace('&amp;', '&').replace('&lt;', '<').replace('&gt;', '>')
                        if t and t != '%':
                            rows.append((rel, n, 'ui:' + prop, t))
                    continue
                if line.lstrip().startswith('//'):
                    continue
                done = set()
                for kind, rx in (('tip', TIP_RE), ('tr', TR_RE), ('msg', MSG_RE)):
                    for m in rx.finditer(line):
                        t = unescape(m.group(1))
                        if t and (t not in done):
                            done.add(t)
                            rows.append((rel, n, kind, t))
    return rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--markdown', action='store_true')
    ap.add_argument('--root', default=os.path.abspath(os.path.join(HERE, '..')))
    a = ap.parse_args()
    rows = scan(a.root)
    if a.markdown:
        print('| Where | Kind | Current wording |\n|---|---|---|')
    for rel, n, kind, t in rows:
        if a.markdown:
            print('| `%s:%d` | %s | %s |' % (rel, n, kind, t.replace('|', '\\|')))
        else:
            print('%s:%d\t%s\t%s' % (rel, n, kind, t))
    print('%d strings' % len(rows), file=sys.stderr)


if __name__ == '__main__':
    main()
