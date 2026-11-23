#!/usr/bin/env python3
# PinglyMotion - fails when a test lists a source file of the shared test
# library itself (CLAUDE.md, docs/DEV.md "Compile faster").
# The files in PINGLYMOTION_SHARED_SRC (src/CMakeLists.txt) are built once, as
# the static library pinglymotion_shared. A test that lists one of them in
# ADD_EXECUTABLE still builds and passes, only slower, so nobody would see it.
# Usage: python3 tools/check_shared_lib.py [repo root]
# Licensed under the GNU GPL version 2 or later.
import os
import re
import sys

MESSAGE = ("Link pinglymotion_shared and do not list the file: it is already "
           "built once in the library (src/CMakeLists.txt, "
           "PINGLYMOTION_SHARED_SRC).")

SOURCE_DIR_VARS = ("CMAKE_SOURCE_DIR", "PROJECT_SOURCE_DIR",
                   "PinglyMotion_SOURCE_DIR", "CMAKE_CURRENT_SOURCE_DIR")


def strip_comments(text):
    """Remove # comments (a # inside a quoted string stays)."""
    out = []
    for line in text.splitlines():
        quoted = False
        for i, ch in enumerate(line):
            if ch == '"':
                quoted = not quoted
            elif ch == "#" and not quoted:
                line = line[:i]
                break
        out.append(line)
    return "\n".join(out)


def call_args(text, name):
    """Yield the argument text of every NAME(...) call, name case-insensitive,
    with balanced parentheses."""
    for m in re.finditer(r"(?<![A-Za-z0-9_])" + name + r"\s*\(", text,
                         re.I):
        depth = 1
        i = m.end()
        while i < len(text) and depth:
            if text[i] == "(":
                depth += 1
            elif text[i] == ")":
                depth -= 1
            i += 1
        yield text[m.end():i - 1]


def words(args):
    return re.findall(r'"[^"]*"|[^\s"]+', args)


def shared_sources(src_cmake_text):
    """The file list of PINGLYMOTION_SHARED_SRC, as written (relative to src/)."""
    text = strip_comments(src_cmake_text)
    for args in call_args(text, "SET"):
        w = words(args)
        if w and w[0] == "PINGLYMOTION_SHARED_SRC":
            return [x.strip('"') for x in w[1:] if not x.startswith("$")]
    raise ValueError("PINGLYMOTION_SHARED_SRC not found in src/CMakeLists.txt")


def variables(text):
    """SET(VAR items) and LIST(APPEND VAR items) of one CMake file."""
    found = {}
    for args in call_args(text, "SET"):
        w = words(args)
        if w:
            found[w[0]] = [x.strip('"') for x in w[1:]]
    for args in call_args(text, "LIST"):
        w = words(args)
        if len(w) >= 2 and w[0].upper() == "APPEND":
            found.setdefault(w[1], []).extend(x.strip('"') for x in w[2:])
    return found


def expand(items, known, depth=0):
    """Replace a word that is exactly ${VAR} (set in the same file) by its items."""
    out = []
    for it in items:
        m = re.fullmatch(r"\$\{(\w+)\}", it)
        if m and m.group(1) in known and depth < 10:
            out.extend(expand(known[m.group(1)], known, depth + 1))
        else:
            out.append(it)
    return out


def normalise(token, cmake_dir_rel):
    """(path, unresolved): path relative to the repo root with / separators.
    A leading source-dir variable is resolved; any other ${...} stays and
    marks the path as unresolved."""
    t = token.replace("\\", "/")
    for var in SOURCE_DIR_VARS:
        base = "" if var != "CMAKE_CURRENT_SOURCE_DIR" else cmake_dir_rel
        if t.startswith("${" + var + "}/"):
            t = (base + "/" if base else "") + t[len(var) + 4:]
            break
    else:
        if not t.startswith("${") and not t.startswith("/"):
            t = (cmake_dir_rel + "/" if cmake_dir_rel else "") + t
    unresolved = "${" in t
    return os.path.normpath(t).replace("\\", "/") if not unresolved else t, \
        unresolved


def violations(shared, cmake_text, cmake_dir_rel):
    """The (shared file, token) pairs that one tests CMakeLists.txt lists in
    ADD_EXECUTABLE."""
    text = strip_comments(cmake_text)
    known = variables(text)
    found = []
    for args in call_args(text, "ADD_EXECUTABLE"):
        for token in expand([x.strip('"') for x in words(args)][1:], known):
            path, unresolved = normalise(token, cmake_dir_rel)
            for rel in shared:
                full = "src/" + rel
                if path == full or path.endswith("/" + full):
                    found.append((rel, token))
                elif unresolved and (path.endswith("/" + rel) and
                                     "/src/" in path + "/"):
                    found.append((rel, token))
    return found


def check(root):
    """List of problem lines; empty when fine."""
    with open(os.path.join(root, "src", "CMakeLists.txt")) as f:
        shared = shared_sources(f.read())
    problems = []
    tests = os.path.join(root, "tests")
    for folder, _dirs, files in sorted(os.walk(tests)):
        if "CMakeLists.txt" not in files:
            continue
        path = os.path.join(folder, "CMakeLists.txt")
        rel_dir = os.path.relpath(folder, root).replace(os.sep, "/")
        with open(path) as f:
            for rel, token in violations(shared, f.read(), rel_dir):
                problems.append("%s/CMakeLists.txt lists %s (%s). %s" % (
                    rel_dir, rel, token, MESSAGE))
    return problems


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..")
    problems = check(root)
    for p in problems:
        print("::error title=Shared test library::" + p)
    if problems:
        return 1
    print("Shared test library: ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
