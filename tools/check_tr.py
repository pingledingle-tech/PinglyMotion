#!/usr/bin/env python3
"""Translation checks (docs/TRANSLATION.md). Warnings only: the exit code is 0 unless --strict.

  python3 tools/check_tr.py                 report user-visible texts in src/ that are not in tr()
  python3 tools/check_tr.py --ts-sync DIR   also run lupdate through the build in DIR and report
                                            when translations/pinglymotion_en.ts is out of date

The first check looks for a text in quotes inside a call that puts text on the screen (setText,
setWindowTitle, setToolTip, addItem, QLabel(...), QMessageBox::warning(...) and so on) that is not
inside tr(), QObject::tr(), QCoreApplication::translate() or QT_TR_NOOP(). It is a line-based
check: it cannot know what a text is for. A text that is not for the user (a key, a file format,
a control command word, an example the user types) is allowed with the comment `// tr-ok` on
the same line. Whole areas that are not translated yet are listed in SKIP below, and in the docs.

Standard library only.
"""

import os
import re
import subprocess
import sys
import xml.etree.ElementTree as ET

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, ".."))
TS_FILE = os.path.join("translations", "pinglymotion_en.ts")

# not translated yet, on purpose (docs/TRANSLATION.md, "Not translated")
SKIP = (
    "src/scriptlab/lsl/",      # the LSL interpreter: messages are about script code
    "src/controlapi.cpp",      # the control commands and their answers (for programs and assistants)
    "src/controlserver.cpp",
    "src/posedescribe.cpp",    # descriptions for assistants
    "src/motiondescribe.cpp",
    "src/animcheck.cpp",
)

CALLS = (
    "setText|setWindowTitle|setToolTip|setStatusTip|setPlaceholderText|setTitle|setLabelText|"
    "setWhatsThis|setInformativeText|setDetailedText|addItem|addAction|addMenu|showMessage|"
    "setHeaderLabels|setHorizontalHeaderLabels|QLabel|QPushButton|QAction|QCheckBox|QRadioButton|"
    "QGroupBox|QToolButton|QMenu|information|warning|critical|question|about|getText|getItem|"
    "getSaveFileName|getOpenFileName|getExistingDirectory|setSuffix|setPrefix|setSpecialValueText"
)
CALL_RE = re.compile(r"\b(?:%s)\s*\(([^;]*)" % CALLS)
LITERAL_RE = re.compile(r'"((?:[^"\\]|\\.)*)"')
WRAPPED_RE = re.compile(r"(?:\btr|\btranslate|QT_TR_NOOP|QT_TRANSLATE_NOOP)\s*\(\s*(?:\"[^\"]*\"\s*,\s*)?$")


def looks_like_text(literal):
    """A sentence or a word the user reads: it starts with a capital letter, or holds words with
    a space. Keys (stop_reason), resource and file paths, command words and "-" do not count."""
    if not re.search(r"[A-Za-z]{2}", literal):
        return False
    if literal[0] in ":/@." or re.fullmatch(r"[a-z0-9_.\-]+", literal):
        return False
    return literal[0].isupper() or bool(re.search(r"[A-Za-z]+ [A-Za-z]+", literal))


def find_untranslated(text):
    """The (line number, line) pairs of one source file that hold an unmarked user-visible text."""
    found = []
    for number, line in enumerate(text.splitlines(), 1):
        stripped = line.strip()
        if stripped.startswith(("//", "*", "/*")) or "tr-ok" in line:
            continue
        match = CALL_RE.search(line)
        if not match:
            continue
        rest = match.group(1)
        for literal in LITERAL_RE.finditer(rest):
            if not looks_like_text(literal.group(1)):
                continue
            if WRAPPED_RE.search(rest[:literal.start()]):
                continue
            # a text inside tr( ... "a" "b" ): the first piece was wrapped, the rest follows it
            if re.search(r"\btr\s*\([^)]*$", rest[:literal.start()]):
                continue
            found.append((number, stripped))
            break
    return found


def scan(root):
    results = []
    src = os.path.join(root, "src")
    for folder, _dirs, files in os.walk(src):
        for name in sorted(files):
            if not name.endswith(".cpp"):
                continue
            path = os.path.join(folder, name)
            rel = os.path.relpath(path, root).replace(os.sep, "/")
            if rel.startswith(SKIP):
                continue
            with open(path, encoding="utf-8", errors="replace") as handle:
                for number, line in find_untranslated(handle.read()):
                    results.append((rel, number, line))
    return sorted(results)


def ts_messages(xml_text):
    """The set of (context, source) pairs in a .ts file."""
    root = ET.fromstring(xml_text)
    pairs = set()
    for context in root.findall("context"):
        name = context.findtext("name", "")
        for message in context.findall("message"):
            pairs.add((name, message.findtext("source", "")))
    return pairs


def ts_sync(build_dir, root):
    """Texts missing from, and left over in, the committed .ts file. Runs lupdate through the build."""
    committed = subprocess.run(["git", "show", "HEAD:" + TS_FILE.replace(os.sep, "/")], cwd=root,
                               capture_output=True, text=True)
    if committed.returncode != 0:
        return None, "translations/pinglymotion_en.ts is not in the last commit"
    run = subprocess.run(["cmake", "--build", build_dir, "--target", "update_translations"],
                         cwd=root, capture_output=True, text=True)
    if run.returncode != 0:
        return None, "update_translations failed (is Qt LinguistTools installed?)\n" + run.stdout[-800:] + run.stderr[-800:]
    with open(os.path.join(root, TS_FILE), encoding="utf-8") as handle:
        fresh = ts_messages(handle.read())
    old = ts_messages(committed.stdout)
    # put the committed file back: this check never changes the working tree
    subprocess.run(["git", "checkout", "--", TS_FILE.replace(os.sep, "/")], cwd=root, capture_output=True)
    return (sorted(fresh - old), sorted(old - fresh)), None


def warn(message, path=None, line=None):
    if os.environ.get("GITHUB_ACTIONS"):
        where = (" file=%s,line=%d" % (path, line)) if path else ""
        print("::warning%s::%s" % (where, message))
    else:
        print(("%s:%d: " % (path, line) if path else "") + "warning: " + message)


def main(argv):
    strict = "--strict" in argv
    problems = 0
    for rel, number, line in scan(ROOT):
        warn("text not in tr(): %s  (add tr(), or `// tr-ok` if it is not for the user)" % line[:100], rel, number)
        problems += 1

    if "--ts-sync" in argv:
        build_dir = argv[argv.index("--ts-sync") + 1]
        result, error = ts_sync(build_dir, ROOT)
        if error:
            warn(error)
            problems += 1
        else:
            added, gone = result
            for context, source in added:
                warn("not in %s yet: [%s] %s" % (TS_FILE, context, source[:80]))
            for context, source in gone:
                warn("left over in %s: [%s] %s" % (TS_FILE, context, source[:80]))
            if added or gone:
                warn("run `cmake --build <build folder> --target update_translations` and commit %s" % TS_FILE)
            problems += len(added) + len(gone)

    print("check_tr: %d warning(s)" % problems)
    return 1 if (strict and problems) else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
