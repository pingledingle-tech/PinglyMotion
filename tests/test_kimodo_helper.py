#!/usr/bin/env python3
"""Tests of the optional Kimodo helper (tools/kimodo/, issue 18) that need no
graphics card and no network: argument parsing, the length, the file names in
the work folder, the token (never printed, stored only in the helper folder),
and a whole "make" with a stand-in for Kimodo. Run: python3 tests/test_kimodo_helper.py"""

import contextlib
import io
import os
import re
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
KIMODO = os.path.join(ROOT, "tools", "kimodo")
sys.path.insert(0, KIMODO)
sys.dont_write_bytecode = True  # no __pycache__ in tools/kimodo

import kimodo_helper as kh  # noqa: E402



def read(path, encoding="utf-8"):
    with open(path, encoding=encoding) as f:
        return f.read()


FAKE_TOKEN = "hf_" + "Ab3" * 12  # the shape of a token, not a real one

# a stand-in for "python -m kimodo.scripts.generate": prints its environment's
# token (as a careless program might), checks the arguments and writes stem.bvh
FAKE_GENERATOR = r'''
import os, sys
args = sys.argv[1:]
print("loading with token " + os.environ.get("HF_TOKEN", ""))
print("text encoder on " + os.environ.get("TEXT_ENCODER_DEVICE", "?"))
stem = args[args.index("--output") + 1]
assert "--bvh" in args and "--bvh_standard_tpose" in args
assert args[args.index("--model") + 1] == "Kimodo-SOMA-RP-v1.2"
if os.environ.get("FAKE_FAIL"):
    print("CUDA out of memory")
    sys.exit(3)
with open(stem + ".bvh", "w") as f:
    f.write("HIERARCHY\nprompt=" + args[0] + "\nduration=" + args[args.index("--duration") + 1] + "\n")
'''


class Env:
    """A helper folder, a work folder and the stand-in generator, all temporary."""

    def __enter__(self):
        self.tmp = tempfile.TemporaryDirectory()
        base = self.tmp.name
        self.home = os.path.join(base, "helper")
        self.work = os.path.join(base, "Documents", "PinglyMotion")
        self.gen = os.path.join(base, "fake_generate.py")
        with open(self.gen, "w") as f:
            f.write(FAKE_GENERATOR)
        self.patch = mock.patch.dict(os.environ, {
            "PINGLYMOTION_KIMODO_HOME": self.home,
            "PINGLYMOTION_CONTROL_FOLDER": self.work,
            "PINGLYMOTION_KIMODO_GENERATOR": self.gen,
        })
        self.patch.start()
        return self

    def __exit__(self, *exc):
        self.patch.stop()
        self.tmp.cleanup()


def run_main(argv):
    lines = []
    err = io.StringIO()
    with contextlib.redirect_stderr(err):
        code = kh.main(argv, out=lambda s="": lines.append(str(s)))
    return code, "\n".join(lines) + err.getvalue()


class Duration(unittest.TestCase):
    def test_forms(self):
        for text, want in [("4s", 4.0), ("4", 4.0), ("4.5 s", 4.5), ("4,5s", 4.5), (" 10 seconds", 10.0),
                           ("1sec", 1.0), ("2.25S", 2.25)]:
            self.assertEqual(kh.parse_duration(text), want, text)

    def test_refused(self):
        for text in ["0.5s", "11s", "-3", "abc", "", "nan", "4m"]:
            with self.assertRaises(ValueError, msg=text):
                kh.parse_duration(text)


class Prompt(unittest.TestCase):
    def test_a_person_is_added(self):
        self.assertEqual(kh.clean_prompt("wave with the right hand"), "A person wave with the right hand.")
        self.assertEqual(kh.clean_prompt("Waves  hello"), "A person waves hello.")
        self.assertEqual(kh.clean_prompt("A person jumps!"), "A person jumps!")
        self.assertEqual(kh.clean_prompt("An old person walks"), "An old person walks.")

    def test_refused(self):
        with self.assertRaises(ValueError):
            kh.clean_prompt("   ")
        with self.assertRaises(ValueError):
            kh.clean_prompt("x" * 301)


class FileNames(unittest.TestCase):
    def test_slug(self):
        self.assertEqual(kh.slug("A person waves with the right hand."), "waves_with_right_hand")
        self.assertEqual(kh.slug("!!!"), "motion")
        self.assertLessEqual(len(kh.slug("word " * 50)), 40)
        self.assertEqual(kh.slug("..\\..\\evil/../name"), "evil_name")

    def test_never_replaces_a_file(self):
        with tempfile.TemporaryDirectory() as d:
            first = kh.output_path(d, "A person waves.")
            self.assertEqual(os.path.basename(first), "kimodo_waves.bvh")
            open(first, "w").close()
            second = kh.output_path(d, "A person waves.")
            self.assertEqual(os.path.basename(second), "kimodo_waves_2.bvh")
            open(second, "w").close()
            self.assertEqual(os.path.basename(kh.output_path(d, "A person waves.")), "kimodo_waves_3.bvh")

    def test_stays_in_the_folder(self):
        with tempfile.TemporaryDirectory() as d:
            for name in ["../../x", "C:\\Windows\\x", "/etc/passwd", "a/b"]:
                p = kh.output_path(d, "waves", name)
                self.assertEqual(os.path.dirname(p), d, name)
                self.assertTrue(p.endswith(".bvh"))

    def test_work_folder_is_documents_pinglymotion(self):
        with mock.patch.dict(os.environ, {}, clear=False):
            os.environ.pop("PINGLYMOTION_CONTROL_FOLDER", None)
            self.assertEqual(os.path.basename(kh.work_folder()), "PinglyMotion")


class Arguments(unittest.TestCase):
    def test_make(self):
        a = kh.build_parser().parse_args(["make", "wave with the right hand", "4s"])
        self.assertEqual((a.command, a.text, a.length), ("make", "wave with the right hand", "4s"))
        a = kh.build_parser().parse_args(["make", "jump", "--seed", "7", "--name", "jump1"])
        self.assertEqual((a.length, a.seed, a.name), ("4s", 7, "jump1"))

    def test_default_is_menu(self):
        self.assertIsNone(kh.build_parser().parse_args([]).command)

    def test_bad_arguments(self):
        with contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit):
                kh.build_parser().parse_args(["make"])
            with self.assertRaises(SystemExit):
                kh.build_parser().parse_args(["make", "x", "--seed", "abc"])

    def test_generate_arguments(self):
        args = kh.build_generate_args("A person waves.", 4.0, "/tmp/s", seed=3, constraints="c.json")
        self.assertEqual(args[0], "A person waves.")
        self.assertEqual(args[args.index("--model") + 1], "Kimodo-SOMA-RP-v1.2")
        self.assertEqual(args[args.index("--duration") + 1], "4")
        self.assertIn("--bvh_standard_tpose", args)
        self.assertEqual(args[args.index("--seed") + 1], "3")
        self.assertEqual(args[args.index("--constraints") + 1], "c.json")

    def test_never_the_research_only_model(self):
        text = read(os.path.join(KIMODO, "kimodo_helper.py"))
        self.assertNotIn("SMPLX-RP", text.replace("never the SMPL-X model", ""))
        self.assertIn("SOMA", kh.MODEL)

    def test_low_vram_environment(self):
        env = kh.kimodo_env(FAKE_TOKEN)
        self.assertEqual(env["TEXT_ENCODER_DEVICE"], "cpu")
        self.assertEqual(env["TEXT_ENCODER_MODE"], "local")
        self.assertEqual(env["HF_TOKEN"], FAKE_TOKEN)


class Token(unittest.TestCase):
    def test_shape(self):
        self.assertTrue(kh.valid_token(FAKE_TOKEN))
        for bad in ["", "hf_short", "abc" * 20, "hf_" + "a" * 30 + " x", "hf_" + "é" * 30]:
            self.assertFalse(kh.valid_token(bad), bad)

    def test_redact(self):
        self.assertNotIn(FAKE_TOKEN, kh.redact("token=" + FAKE_TOKEN + " end", FAKE_TOKEN))
        other = "hf_" + "Zz9" * 10
        self.assertNotIn(other, kh.redact("old " + other, FAKE_TOKEN))
        self.assertEqual(kh.redact("nothing here", None), "nothing here")

    def test_stored_only_in_the_helper_folder(self):
        with Env() as e:
            self.assertIsNone(kh.read_token())
            kh.save_token(FAKE_TOKEN)
            self.assertEqual(kh.read_token(), FAKE_TOKEN)
            self.assertTrue(kh.token_path().startswith(e.home))
            if os.name != "nt":
                self.assertEqual(os.stat(kh.token_path()).st_mode & 0o777, 0o600)
            self.assertFalse(os.path.exists(e.work))
            self.assertTrue(kh.forget_token())
            self.assertIsNone(kh.read_token())

    def test_login_never_prints_the_token(self):
        for probe_result, want_ok in [("ok", True), ("offline", True), ("no-licence", False)]:
            with Env():
                lines = []
                opened = []
                with mock.patch("builtins.input", return_value=""):
                    ok = kh.run_login(out=lambda s="": lines.append(str(s)), ask=lambda prompt: FAKE_TOKEN,
                                      open_page=opened.append, probe=lambda t: probe_result)
                self.assertEqual(ok, want_ok, probe_result)
                self.assertNotIn(FAKE_TOKEN, "\n".join(lines))
                self.assertEqual(opened, [kh.LLAMA_PAGE, kh.TOKEN_PAGE])
                self.assertEqual(kh.read_token(), FAKE_TOKEN)

    def test_login_bad_token_is_not_saved(self):
        with Env():
            lines = []
            with mock.patch("builtins.input", return_value=""):
                ok = kh.run_login(out=lambda s="": lines.append(str(s)), ask=lambda prompt: "hf_" + "Qq1" * 10,
                                  open_page=lambda url: None, probe=lambda t: "bad-token")
            self.assertFalse(ok)
            self.assertIsNone(kh.read_token())
            self.assertNotIn("Qq1Qq1Qq1", "\n".join(lines))

    def test_licence_page_is_metas_llama3(self):
        self.assertEqual(kh.LLAMA_PAGE, "https://huggingface.co/meta-llama/Meta-Llama-3-8B-Instruct")
        self.assertTrue(kh.GATED_PROBE.startswith(kh.LLAMA_PAGE + "/"))


class Make(unittest.TestCase):
    def test_make_writes_the_bvh_into_the_work_folder(self):
        with Env() as e:
            kh.save_token(FAKE_TOKEN)
            code, out = run_main(["make", "wave with the right hand", "4s"])
            self.assertEqual(code, 0, out)
            self.assertNotIn(FAKE_TOKEN, out)
            self.assertIn("hf_***", out)  # the stand-in printed it, the helper blanked it out
            self.assertIn("text encoder on cpu", out)
            files = os.listdir(e.work)
            self.assertEqual(files, ["kimodo_wave_with_right_hand.bvh"])
            with open(os.path.join(e.work, files[0])) as f:
                body = f.read()
            self.assertIn("prompt=A person wave with the right hand.", body)
            self.assertIn("duration=4", body)
            # again: a second file, the first one is kept
            code, out = run_main(["make", "wave with the right hand", "4"])
            self.assertEqual(code, 0, out)
            self.assertEqual(sorted(os.listdir(e.work)),
                             ["kimodo_wave_with_right_hand.bvh", "kimodo_wave_with_right_hand_2.bvh"])

    def test_make_without_token(self):
        with Env() as e:
            code, out = run_main(["make", "jump", "2s"])
            self.assertEqual(code, 3)
            self.assertFalse(os.path.exists(e.work))

    def test_make_bad_length(self):
        with Env():
            kh.save_token(FAKE_TOKEN)
            code, out = run_main(["make", "jump", "30s"])
            self.assertEqual(code, 2)
            self.assertIn("between 1 and 10 seconds", out)

    def test_failed_generation_leaves_nothing(self):
        with Env() as e:
            kh.save_token(FAKE_TOKEN)
            with mock.patch.dict(os.environ, {"FAKE_FAIL": "1"}):
                code, out = run_main(["make", "jump", "2s"])
            self.assertEqual(code, 4)
            self.assertIn("CUDA out of memory", out)
            self.assertNotIn(FAKE_TOKEN, out)
            self.assertEqual(os.listdir(e.work), [])

    def test_where_and_forget(self):
        with Env() as e:
            code, out = run_main(["where"])
            self.assertEqual(code, 0)
            self.assertIn(e.home, out)
            self.assertIn(e.work, out)
            code, out = run_main(["forget-token"])
            self.assertIn("no stored token", out)


class Machine(unittest.TestCase):
    def test_nvidia_smi(self):
        gpus = kh.parse_nvidia_smi("NVIDIA GeForce RTX 2080 Ti, 11264, 566.36\nbroken line\n")
        self.assertEqual(gpus, [{"name": "NVIDIA GeForce RTX 2080 Ti", "memory_mb": 11264, "driver": "566.36"}])
        self.assertEqual(kh.driver_major("566.36"), 566)
        self.assertEqual(kh.driver_major("x"), 0)

    def test_check_without_a_card(self):
        with Env():
            lines = []
            with mock.patch("shutil.which", return_value=None):
                ok = kh.run_check(out=lambda s="": lines.append(str(s)))
            self.assertFalse(ok)
            self.assertIn("no NVIDIA card", "\n".join(lines))


class Scripts(unittest.TestCase):
    def test_python_compiles(self):
        path = os.path.join(KIMODO, "kimodo_helper.py")
        compile(read(path), path, "exec")

    def test_help_runs(self):
        r = subprocess.run([sys.executable, "-B", os.path.join(KIMODO, "kimodo_helper.py"), "--help"],
                           capture_output=True, text=True, check=True)
        self.assertIn("make", r.stdout)

    def test_setup_script_pins_and_checks_the_installer(self):
        ps1 = read(os.path.join(KIMODO, "kimodo_setup.ps1"))
        version = re.search(r"\$UvVersion = '([0-9.]+)'", ps1).group(1)
        sha = re.search(r"\$UvSha256 = '([0-9a-f]{64})'", ps1)
        self.assertIsNotNone(sha)
        self.assertIn("uv-%s-py3-none-win_amd64.whl" % version, ps1)
        self.assertIn("Get-FileHash -Algorithm SHA256", ps1)
        self.assertTrue(ps1.isascii())
        # braces balance (a cheap syntax check without PowerShell)
        self.assertEqual(ps1.count("{"), ps1.count("}"))
        self.assertEqual(ps1.count("("), ps1.count(")"))
        self.assertNotIn("Invoke-Expression", ps1)
        self.assertNotIn("iex ", ps1)

    def test_bat_starts_the_setup(self):
        bat = read(os.path.join(KIMODO, "PinglyMotion-Kimodo.bat"), "ascii")
        self.assertIn('-File "%~dp0kimodo_setup.ps1" %*', bat)
        self.assertIn("-ExecutionPolicy Bypass", bat)

    def test_versions_are_in_the_readme(self):
        readme = read(os.path.join(KIMODO, "README.md"))
        for text in (kh.MODEL, kh.KIMODO_COMMIT, kh.TORCH_PACKAGE, "NVIDIA Open Model License",
                     "Llama 3", "research"):
            self.assertIn(text, readme)

    def test_nothing_of_kimodo_is_committed(self):
        allowed = {"README.md", "PinglyMotion-Kimodo.bat", "kimodo_setup.ps1", "kimodo_helper.py"}
        names = {n for n in os.listdir(KIMODO) if n != "__pycache__"}
        self.assertEqual(names, allowed)


if __name__ == "__main__":
    unittest.main()
