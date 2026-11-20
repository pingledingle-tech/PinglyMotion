#!/usr/bin/env python3
"""
Smoke run with screenshots (issue 129): starts the packed PinglyMotion with remote
control on, asks for its state, opens example files and takes a picture after each
one, then ends the program. The pictures (PNG), the JSON answers and a short report
go to the output folder, which the build uploads as an artifact. An AI session can
look at the pictures of a pull request before the project owner tests it.

--open-dir opens every file the control command 'open_file' can open (.dingle, .bvh,
.avm), found recursively under that folder, in the same running program (no restart
per file); this is "does it open without crashing, does the state look sane" for every
example and test-motion file, not a judgement of whether an animation looks natural
(that stays for the project owner). A file with another extension (.prp, .glb, ...) is
left out and counted in the report; the control protocol has no command to open those
and this job does not add one.

Only the Python standard library is used (the Python on the GitHub runner is enough).

Usage:
  smoke_screenshots.py --app dist/PinglyMotion.exe --folder dist/examples
                       --open-dir . --out smoke

  --folder is the folder remote control may open files from (PINGLYMOTION_CONTROL_FOLDER);
  --open and --open-dir paths are relative to it. --cwd is where the program starts (it
  looks for ./data there; default: the program's own folder, which is right for the
  Windows zip).

Exit code 0 when the program answered, even when a picture is empty (the report says so).
Exit code 1 when the program crashes, never answers, or the run hits --timeout.
"""

import argparse
import base64
import collections
import json
import os
import struct
import subprocess
import sys
import tempfile
import threading
import time
import urllib.error
import urllib.request
import zlib


class SmokeFailure(Exception):
    """The program crashed, did not answer, or the run timed out: the step fails."""


# file types the control command 'open_file' can open (src/controlapi.cpp); a file with
# another extension (.prp, .glb, .gltf, .md, ...) is left out and counted as skipped
OPENABLE_EXTENSIONS = {".dingle", ".bvh", ".avm"}


def discover(folder, relative_dir):
    """Every openable file under <folder>/<relative_dir>, found recursively (all
    subfolders), as paths relative to <folder> with forward slashes, sorted. Also
    returns how many files of each other extension were found but left out, so the
    report can say what was skipped and why."""
    base = os.path.join(folder, relative_dir)
    found = []
    skipped = collections.Counter()
    for root, dirs, files in os.walk(base):
        dirs.sort()
        for name in sorted(files):
            ext = os.path.splitext(name)[1].lower()
            relative = os.path.relpath(os.path.join(root, name), folder).replace(os.sep, "/")
            if ext in OPENABLE_EXTENSIONS:
                found.append(relative)
            else:
                skipped[ext or "(no extension)"] += 1
    return sorted(found), skipped


def decode_png(data):
    """A small PNG reader for the checks below: 8-bit grey, grey+alpha, RGB and RGBA, not
    interlaced. Returns (width, height, channels, rows as bytes) or None if it cannot."""
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        return None
    position = 8
    width = height = depth = colour = interlace = None
    idat = []
    while position + 8 <= len(data):
        length, kind = struct.unpack(">I4s", data[position:position + 8])
        body = data[position + 8:position + 8 + length]
        position += 12 + length
        if kind == b"IHDR":
            width, height, depth, colour, _, _, interlace = struct.unpack(">IIBBBBB", body)
        elif kind == b"IDAT":
            idat.append(body)
        elif kind == b"IEND":
            break
    channels = {0: 1, 2: 3, 4: 2, 6: 4}.get(colour)
    if not width or not channels or depth != 8 or interlace != 0:
        return None
    raw = zlib.decompress(b"".join(idat))
    stride = width * channels
    rows = []
    previous = bytearray(stride)
    offset = 0
    for _ in range(height):
        kind = raw[offset]
        line = bytearray(raw[offset + 1:offset + 1 + stride])
        offset += 1 + stride
        if kind == 1:
            for i in range(channels, stride):
                line[i] = (line[i] + line[i - channels]) & 255
        elif kind == 2:
            for i in range(stride):
                line[i] = (line[i] + previous[i]) & 255
        elif kind == 3:
            for i in range(stride):
                left = line[i - channels] if i >= channels else 0
                line[i] = (line[i] + ((left + previous[i]) >> 1)) & 255
        elif kind == 4:
            for i in range(stride):
                a = line[i - channels] if i >= channels else 0
                b = previous[i]
                c = previous[i - channels] if i >= channels else 0
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                if pa <= pb and pa <= pc:
                    predictor = a
                elif pb <= pc:
                    predictor = b
                else:
                    predictor = c
                line[i] = (line[i] + predictor) & 255
        elif kind != 0:
            return None
        rows.append(bytes(line))
        previous = line
    return width, height, channels, rows


def describe_picture(data):
    """What the picture holds, in words an AI or a person can read: how much of it differs
    from the background (the top left corner's colour) and how many colours it has."""
    decoded = decode_png(data)
    if decoded is None:
        return {"analysed": False, "note": "not analysed (unusual PNG format)"}
    width, height, channels, rows = decoded
    background = rows[0][:channels]
    differing = 0
    colours = collections.Counter()
    for row in rows:
        for x in range(0, len(row), channels):
            pixel = row[x:x + channels]
            if pixel != background:
                differing += 1
            colours[pixel] += 1
    total = width * height
    fraction = differing / total if total else 0.0
    looks_empty = len(colours) < 3 or fraction < 0.001
    note = "looks empty (one flat colour)" if looks_empty else "shows something (%.1f%% of the pixels differ from the background, %d colours)" % (fraction * 100, len(colours))
    return {"analysed": True, "width": width, "height": height, "differing_fraction": round(fraction, 4),
            "colours": len(colours), "looks_empty": looks_empty, "note": note}


class Run:
    def __init__(self, options):
        self.options = options
        self.out = os.path.abspath(options.out)
        os.makedirs(self.out, exist_ok=True)
        self.lines = []          # the report, line by line
        self.pictures = []       # (name, looks_empty) per screenshot taken
        self.answers = []        # every command and its answer
        self.app = None
        self.info = None
        self.deadline = time.monotonic() + options.timeout
        self.timed_out = False
        self.terminated_by_us = False
        self.program_log = os.path.join(self.out, "program.log")

    def say(self, line):
        print(line, flush=True)
        self.lines.append(line)

    def remaining(self):
        return max(0.0, self.deadline - time.monotonic())

    def watchdog(self):
        """Ends the program at the deadline so a hang never blocks the job."""
        while self.remaining() > 0:
            if self.app is not None and self.app.poll() is not None:
                return
            time.sleep(0.5)
        self.timed_out = True
        if self.app is not None and self.app.poll() is None:
            self.app.kill()

    def start(self):
        options = self.options
        app_path = os.path.abspath(options.app)
        cwd = os.path.abspath(options.cwd) if options.cwd else os.path.dirname(app_path)
        folder = os.path.abspath(options.folder)
        self.folder = folder
        self.scratch = []
        work = tempfile.mkdtemp(prefix="pinglymotion-smoke-")
        self.control_file = os.path.join(work, "control.json")
        env = dict(os.environ,
                   PINGLYMOTION_CONTROL_FILE=self.control_file,
                   PINGLYMOTION_CONTROL_FOLDER=folder,
                   QT_LOGGING_TO_CONSOLE="1",
                   QT_FORCE_STDERR_LOGGING="1")
        if sys.platform.startswith("win"):
            # the runner has no graphics card: Qt's software OpenGL (opengl32sw.dll from the zip)
            env.setdefault("QT_OPENGL", "software")
            software = os.path.join(os.path.dirname(app_path), "opengl32sw.dll")
            self.say("opengl32sw.dll next to the program: %s" % ("yes" if os.path.exists(software) else "NO (the 3D view will probably stay empty)"))
        self.say("program: %s (started in %s)" % (os.path.basename(app_path), os.path.basename(cwd) or cwd))
        self.say("files folder: %s" % os.path.basename(folder))
        if not os.path.isfile(app_path):
            raise SmokeFailure("the program does not exist: " + os.path.basename(app_path))
        log = open(self.program_log, "wb")
        self.app = subprocess.Popen([app_path, "--control"], env=env, cwd=cwd, stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT)
        threading.Thread(target=self.watchdog, daemon=True).start()

        waited = 0.0
        while True:
            if self.app.poll() is not None:
                raise SmokeFailure("the program ended with exit code %s before remote control was ready" % self.app.returncode)
            try:
                with open(self.control_file) as file:
                    info = json.load(file)
                if info.get("pid") == self.app.pid and info.get("url") and info.get("token"):
                    self.info = info
                    break
            except (OSError, ValueError):
                pass
            if waited > min(90.0, self.remaining()):
                raise SmokeFailure("remote control did not start within %d seconds (no control file)" % int(waited))
            time.sleep(0.5)
            waited += 0.5
        self.say("remote control ready after %.1f s" % waited)
        # the window may still be drawing its first frame: give it a moment
        if self.options.settle > 0:
            time.sleep(min(self.options.settle, self.remaining()))

    def command(self, body):
        """One command over HTTP with the key; raises SmokeFailure if the program does not answer."""
        headers = {"Authorization": "Bearer " + self.info["token"], "Content-Type": "application/json",
                   "X-PinglyMotion-Client": "CI smoke run"}
        request = urllib.request.Request(self.info["url"] + "/api", data=json.dumps(body).encode(), headers=headers)
        try:
            with urllib.request.urlopen(request, timeout=min(60.0, max(1.0, self.remaining()))) as response:
                answer = json.load(response)
        except urllib.error.HTTPError as error:
            try:
                answer = json.load(error)
            except ValueError:
                answer = {"ok": False, "error": "HTTP %d" % error.code}
        except Exception as error:  # no answer at all: refused, timed out, connection reset
            if self.app.poll() is not None:
                raise SmokeFailure("the program ended with exit code %s while '%s' was sent" % (self.app.returncode, body.get("command")))
            raise SmokeFailure("no answer to '%s': %s" % (body.get("command"), error))
        kept = dict(answer)
        if "png_base64" in kept:
            kept["png_base64"] = "(%d bytes of base64, see the PNG file)" % len(kept["png_base64"])
        self.answers.append({"command": body, "answer": kept})
        return answer

    def picture(self, name, label):
        """A screenshot into <name>.png, with a description for the report and a small preview in the log."""
        answer = self.command({"command": "screenshot", "width": self.options.width})
        if not answer.get("ok"):
            self.say("  screenshot %s: the program says: %s" % (name, answer.get("error", "failed")))
            return
        data = base64.b64decode(answer["png_base64"])
        path = os.path.join(self.out, name + ".png")
        with open(path, "wb") as file:
            file.write(data)
        found = describe_picture(data)
        self.pictures.append((name, found.get("looks_empty", False)))
        self.say("  screenshot %s.png (%dx%d, %d bytes): %s" % (name, answer.get("width", 0), answer.get("height", 0), len(data), found["note"]))
        if self.options.preview_width > 0:
            preview = self.command({"command": "screenshot", "width": self.options.preview_width})
            if preview.get("ok"):
                text = preview["png_base64"]
                print("--- preview %s.png (%s, PNG as base64, %d px wide) begins ---" % (name, label, self.options.preview_width))
                for start in range(0, len(text), 76):
                    print(text[start:start + 76])
                print("--- preview %s.png ends ---" % name, flush=True)

    def state_line(self, answer):
        if not answer.get("ok"):
            return "state failed: " + answer.get("error", "?")
        return "state ok: file=%s frames=%s fps=%s frame=%s figure=%s" % (
            os.path.basename(str(answer.get("file", ""))) or "(new animation)", answer.get("frames"), answer.get("fps"),
            answer.get("frame"), answer.get("figure"))

    def files_to_open(self):
        """The explicit --open files, in order, then every openable file found under each
        --open-dir (recursively), skipping a path already in the list. Files skipped for
        having an unsupported extension (.prp, .glb, ...) are logged, not opened: the
        control command 'open_file' cannot open them (src/controlapi.cpp), and this job
        does not grow the control protocol to make that possible."""
        opens = list(self.options.open)
        seen = set(opens)
        for directory in self.options.open_dir:
            found, skipped = discover(self.folder, directory)
            added = [relative for relative in found if relative not in seen]
            seen.update(added)
            opens.extend(added)
            total_skipped = sum(skipped.values())
            detail = ", ".join("%s: %d" % item for item in sorted(skipped.items())) if skipped else "none"
            self.say("open-dir %s: %d openable file(s) added, %d file(s) skipped (not .dingle/.bvh/.avm: %s)" %
                     (directory, len(added), total_skipped, detail))
        return opens

    def run(self):
        self.start()
        self.say("1. start")
        answer = self.command({"command": "state"})
        if not answer.get("ok"):
            raise SmokeFailure("the first 'state' failed: " + answer.get("error", "?"))
        self.say("  " + self.state_line(answer))
        self.picture("start", "start state")

        opens = self.files_to_open()
        opened_ok = []
        opened_failed = []
        for number, relative in enumerate(opens, start=2):
            self.say("%d. open %s" % (number, relative))
            # the start animation is not changed, so no "unsaved changes" refusal; after an opened
            # file nothing was changed either
            answer = self.command({"command": "open_file", "path": relative})
            if not answer.get("ok"):
                self.say("  open_file failed: %s" % answer.get("error", "?"))
                opened_failed.append(relative)
                continue
            opened_ok.append(relative)
            state = self.command({"command": "state"})
            # a file opens on its reference frame; the middle frame shows the motion itself
            # (set_frame changes no animation data, so the next open_file is not refused)
            frames = state.get("frames") or 1
            if frames > 2:
                self.command({"command": "set_frame", "frame": frames // 2 + 1})
                state = self.command({"command": "state"})
            self.say("  " + self.state_line(state))
            # the relative path, not just the file name, keeps names unique (two files
            # under different subfolders can share a file name, e.g. the CMU walk clip)
            safe = relative.replace("/", "_")
            name = os.path.splitext(safe)[0] + "_" + os.path.splitext(safe)[1].lstrip(".")
            self.picture(name, relative)
            # an old .avm file opens under its new .dingle name and counts as changed, so the
            # next open_file would be refused: save it once to a scratch file (removed at the end)
            if state.get("unsaved_changes"):
                scratch = "smoke-scratch-%d.dingle" % number
                self.scratch.append(os.path.join(self.folder, scratch))
                answer = self.command({"command": "save_file", "path": scratch})
                if not answer.get("ok"):
                    self.say("  save to a scratch file failed: %s" % answer.get("error", "?"))

        for path in self.scratch:
            try:
                os.remove(path)
            except OSError:
                pass

        if opens:
            self.say("opened %d of %d file(s); %d failed to open%s" % (
                len(opened_ok), len(opens), len(opened_failed),
                ": " + ", ".join(opened_failed) if opened_failed else ""))

        self.end()

    def end(self):
        # the control API has no quit command: end the program from outside
        if self.app.poll() is None:
            self.terminated_by_us = True
            self.app.terminate()
            try:
                self.app.wait(timeout=15)
            except subprocess.TimeoutExpired:
                self.app.kill()
                self.app.wait(timeout=15)
            self.say("program ended by the script after the run")
        else:
            raise SmokeFailure("the program ended on its own with exit code %s" % self.app.returncode)

    def finish(self, failure):
        if self.app is not None and self.app.poll() is None:
            self.app.kill()
            self.app.wait(timeout=15)
        if self.timed_out:
            failure = "the run hit the time limit of %d seconds; the program was ended" % self.options.timeout
        # the renderer line from the program's own output tells how the 3D view was drawn
        try:
            with open(self.program_log, "rb") as file:
                program_lines = file.read().decode("utf-8", "replace").splitlines()
        except OSError:
            program_lines = []
        for line in program_lines:
            if "3D view" in line or "OpenGL" in line:
                self.say("program says: " + line.strip())
        drawn = [name for name, empty in self.pictures if not empty]
        if self.pictures:
            self.say("pictures: %d of %d show something%s" % (len(drawn), len(self.pictures),
                     "" if len(drawn) == len(self.pictures) else " (an empty picture means the 3D view could not be drawn on this machine, see 'program says' above; the run itself is not failed by it)"))
        self.say("RESULT: " + ("FAIL: " + failure if failure else "OK (the program started, answered and ended)"))
        with open(os.path.join(self.out, "report.txt"), "w") as file:
            file.write("PinglyMotion smoke run with screenshots (issue 129)\n")
            file.write("\n".join(self.lines) + "\n")
        with open(os.path.join(self.out, "answers.json"), "w") as file:
            json.dump(self.answers, file, indent=1)
        if failure:
            print("::error::Smoke run failed: " + failure)
            return 1
        return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--app", required=True, help="the program (PinglyMotion.exe or PinglyMotion)")
    parser.add_argument("--folder", required=True, help="folder remote control may open files from")
    parser.add_argument("--open", action="append", default=[], help="file to open, relative to --folder (repeat)")
    parser.add_argument("--open-dir", action="append", default=[], help="folder, relative to --folder, to open every .dingle/.bvh/.avm file from, recursively (repeat)")
    parser.add_argument("--out", default="smoke", help="output folder for PNGs, answers and the report")
    parser.add_argument("--cwd", default=None, help="where the program starts (default: its own folder)")
    parser.add_argument("--timeout", type=int, default=240, help="seconds for the whole run")
    parser.add_argument("--settle", type=float, default=2.0, help="seconds to wait after remote control is ready, before the first picture")
    parser.add_argument("--width", type=int, default=800, help="screenshot width in pixels")
    parser.add_argument("--preview-width", type=int, default=320, help="width of the base64 preview in the log; 0 for none")
    options = parser.parse_args()

    run = Run(options)
    failure = None
    try:
        run.run()
    except SmokeFailure as error:
        failure = str(error)
    return run.finish(failure)


if __name__ == "__main__":
    sys.exit(main())
