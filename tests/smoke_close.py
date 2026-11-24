#!/usr/bin/env python3
"""
Closing an animation and removing a figure, then using the program (job R2).

Starts the real PinglyMotion program in Expert mode with one animation, adds a second figure
through File > Add a second figure, removes figure 1 of 2 with File > Close animation, closes the
last animation (the empty state), and after each step sends the control commands that play,
stop, step, set loop points and read. The program must still run, answer, and (under the
sanitizers) report nothing. Needs a display and xdotool: run with xvfb-run.

Usage: tests/smoke_close.py path/to/pinglymotion
Without xdotool the test says so and passes, unless PINGLYMOTION_REQUIRE_XDOTOOL is set.
"""

import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
failed = []


def check(name, condition, detail=""):
    print(("OK    " if condition else "FAIL  ") + name + ("" if condition else "  " + str(detail)[:400]))
    if not condition:
        failed.append(name)


def xdo(*args):
    subprocess.run(["xdotool"] + [str(a) for a in args], check=True, timeout=20)


def click(x, y):
    xdo("mousemove", x, y)
    time.sleep(0.4)
    xdo("click", 1)
    time.sleep(1.5)


def shot(name):
    """for a person who debugs this test: PINGLYMOTION_SHOTS=<folder> saves a picture of the screen"""
    folder = os.environ.get("PINGLYMOTION_SHOTS")
    if folder:
        subprocess.run(["import", "-window", "root", os.path.join(folder, name + ".png")], timeout=20)


def key(*keys):
    for name in keys:
        xdo("key", name)
        time.sleep(0.4)


def close_animation():
    click(19, 8)
    click(80, 313)  # File > Close animation


def main():
    if shutil.which("xdotool") is None:
        if os.environ.get("PINGLYMOTION_REQUIRE_XDOTOOL"):
            print("FAIL  xdotool is needed")
            return 1
        print("SKIP  xdotool is not installed")
        return 0
    app_path = os.path.abspath(sys.argv[1])
    work = tempfile.mkdtemp(prefix="pinglymotion-close-")
    home = os.path.join(work, "home")
    folder = os.path.join(work, "files")
    os.makedirs(os.path.join(home, ".config", "PinglyMotion"))
    os.makedirs(folder)
    # Expert mode, no welcome card: the program reads these as its own settings
    with open(os.path.join(home, ".config", "PinglyMotion", "PinglyMotion-Development.ini"), "w") as ini:
        ini.write("[user_settings]\n1.0\\expert_mode=true\n1.0\\welcome_seen=true\n")
    first = os.path.join(folder, "first.dingle")
    second = os.path.join(folder, "second.dingle")
    shutil.copy(os.path.join(ROOT, "examples", "wave.dingle"), first)
    shutil.copy(os.path.join(ROOT, "examples", "jump.dingle"), second)

    control_file = os.path.join(work, "control.json")
    env = dict(os.environ, HOME=home, XDG_CONFIG_HOME=os.path.join(home, ".config"), QT_QPA_PLATFORM="xcb",
               PINGLYMOTION_CONTROL_FILE=control_file, PINGLYMOTION_CONTROL_FOLDER=folder)
    log = open(os.path.join(work, "app.log"), "w")
    app = subprocess.Popen([app_path, "--control", first], env=env, cwd=os.path.join(ROOT, "bin"), stdout=log, stderr=subprocess.STDOUT)
    try:
        info = None
        for _ in range(90):
            try:
                with open(control_file) as handle:
                    info = json.load(handle)
                if info.get("pid") == app.pid:
                    break
            except (OSError, ValueError):
                pass
            time.sleep(0.5)
        check("control file appears", bool(info) and info.get("pid") == app.pid, info)
        if not info:
            return 1
        time.sleep(6)
        # there is no window manager here, so no key reaches the program: the test uses the
        # mouse for menus and buttons, and the control commands for playback and loop points

        def api(request):
            data = json.dumps(request).encode()
            req = urllib.request.Request(info["url"] + "/api", data=data, headers={"Authorization": "Bearer " + info["token"]})
            try:
                with urllib.request.urlopen(req, timeout=20) as response:
                    return json.load(response)
            except urllib.error.HTTPError as error:
                return json.load(error)

        def alive(step):
            check("the program still runs " + step, app.poll() is None, "exit code %s" % app.poll())
            return app.poll() is None

        def use(step, open_animation):
            """the control commands that play, step, loop and read: the program's own playback slots"""
            if not alive("before the control commands " + step):
                return False
            answers = [api({"command": c}) for c in ("state", "play", "stop")]
            answers.append(api({"command": "set_frame", "frame": 2}))
            answers.append(api({"command": "set_loop", "in": 2, "out": 5}))
            answers.append(api({"command": "set_loop", "off": True}))
            answers.append(api({"command": "set_fps", "fps": 20}))
            answers.append(api({"command": "set_frame_count", "frames": 30}))
            answers.append(api({"command": "get_pose"}))
            answers.append(api({"command": "keys"}))
            answers.append(api({"command": "describe_pose"}))
            check("every control command answers " + step, all(isinstance(a, dict) and "ok" in a for a in answers), answers)
            if open_animation:
                check("the commands work on the open animation " + step, answers[0].get("ok") is True, answers[0])
            else:
                check("the commands refuse in the empty state " + step, all(a.get("ok") is False for a in answers), answers)
            api({"command": "stop"})
            if open_animation:
                # the commands changed the animation: save it, so closing does not ask first
                saved = api({"command": "save_file", "path": step.replace(" ", "_") + ".dingle"})
                check("the changes are saved " + step, saved.get("ok") is True, saved)
            return alive("after the control commands " + step)

        check("one animation is open", api({"command": "state"}).get("file", "").endswith("first.dingle"), api({"command": "state"}))
        shot("start")

        # File > Add a second figure... > From a file... > the file dialog
        click(19, 8)
        click(80, 241)
        click(682, 302)
        time.sleep(1)
        xdo("type", "--delay", "40", second)
        key("Return")
        time.sleep(3)
        check("the second figure is added", app.poll() is None)
        # figure 1 is made active (Alt+1), then File > Close animation takes it out
        click(451, 404)  # "Make active" on the first figure's header row
        shot("two_figures")
        check("figure 1 is active", api({"command": "state"}).get("file", "").endswith("first.dingle"), api({"command": "state"}))
        if alive("with two figures") and use("with two figures", True):
            before = api({"command": "state"}).get("file", "")
            close_animation()
            time.sleep(2)
            shot("after_first_close")
            if alive("after figure 1 of 2 was removed"):
                state = api({"command": "state"})
                check("the other figure is the one left", state.get("file") and state.get("file") != before, (before, state))
                if use("with one figure left", True):
                    close_animation()
                    time.sleep(2)
                    shot("after_second_close")
                    if alive("after the last animation was closed"):
                        use("in the empty state", False)
                        click(19, 8)
                        click(75, 31)  # File > New animation
                        time.sleep(2)
                        alive("after a new animation")
        return 0 if not failed else 1
    finally:
        if app.poll() is None:
            app.terminate()
            try:
                app.wait(timeout=15)
            except subprocess.TimeoutExpired:
                app.kill()
        log.close()
        if failed:
            with open(os.path.join(work, "app.log")) as handle:
                print(handle.read()[-3000:])
        shutil.rmtree(work, ignore_errors=True)
        print("\n%s" % ("ALL OK" if not failed else "FAILED: " + ", ".join(failed)))


if __name__ == "__main__":
    code = main()
    sys.exit(1 if failed else (code or 0))
