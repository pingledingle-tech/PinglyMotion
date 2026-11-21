#!/usr/bin/env python3
"""PinglyMotion helper for NVIDIA Kimodo (text-to-motion), outside the program.

Optional. Makes a BVH animation from a sentence ("wave with the right hand")
with the Kimodo-SOMA model on the user's own NVIDIA graphics card, and puts it
in the PinglyMotion work folder (Documents\\PinglyMotion). PinglyMotion then
opens it with File > Import BVH from another skeleton, which knows Kimodo's
SOMA skeleton.

Normally started by PinglyMotion-Kimodo.bat (Windows), which first sets up a
private Python with the "uv" tool (kimodo_setup.ps1). Commands:

  check                     graphics card, driver, memory and disk
  login                     Meta's Llama 3 licence and your Hugging Face token
  install                   PyTorch, Kimodo and the models (large download)
  make "TEXT" [4s]          make a motion; the BVH goes to the work folder
  menu                      ask what to make, again and again (the default)
  forget-token              delete the stored token
  where                     show the folders this helper uses

Standard library only, so the same file runs before anything is installed.
The Hugging Face token is read without echo, stored only in this helper's own
folder on this computer, given to Kimodo as an environment variable, and
never printed or logged (every line Kimodo prints is checked for it).
See tools/kimodo/README.md for the licences and versions.
"""

import argparse
import ctypes
import getpass
import hashlib
import os
import re
import shutil
import subprocess
import sys
import tempfile
import urllib.error
import urllib.request
import webbrowser

# ---- versions (change together, and update tools/kimodo/README.md) ----------

MODEL = "Kimodo-SOMA-RP-v1.2"  # NVIDIA Open Model License; never the SMPL-X model
KIMODO_COMMIT = "e1c219c99b4b94ba48be1fb6a1cbc64ecee4c954"  # nv-tlabs/kimodo, 15 Oct 2026
KIMODO_PACKAGE = "kimodo @ https://github.com/nv-tlabs/kimodo/archive/%s.zip" % KIMODO_COMMIT
TORCH_PACKAGE = "torch==2.8.0"
TORCH_INDEX = "https://download.pytorch.org/whl/cu126"  # CUDA 12.6 builds still include RTX 20xx (Turing)
PYPI_INDEX = "https://pypi.org/simple"
# every package, Kimodo and PyTorch included, pinned and checked by SHA-256 (issue 247);
# made by kimodo_lock.py compile in the workflow kimodo-lock.yml
LOCK_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "requirements.lock.txt")
# the model is downloaded at this exact Hugging Face commit, never "whatever is newest" (issue 247)
MODEL_REPO = "nvidia/" + MODEL
MODEL_REVISION = "0e3c89024d896064b6851201373f7834784a386b"

LLAMA_PAGE = "https://huggingface.co/meta-llama/Meta-Llama-3-8B-Instruct"
TOKEN_PAGE = "https://huggingface.co/settings/tokens/new?tokenType=read"
GATED_PROBE = LLAMA_PAGE + "/resolve/main/config.json"

# ---- limits -------------------------------------------------------------------

MIN_SECONDS = 1.0
MAX_SECONDS = 10.0  # Kimodo makes at most 10 s per prompt
MIN_VRAM_MB = 6000  # under 3 GB is used with the text encoder on the processor
MIN_DRIVER = 528  # CUDA 12 on Windows
MIN_RAM_GB = 20  # the text encoder (Llama 3 8B, bfloat16) needs about 16 GB of RAM
GOOD_RAM_GB = 32
MIN_DISK_GB = 40  # PyTorch, Kimodo, the Llama text encoder (about 16 GB) and the model
MAX_PROMPT = 300

TOKEN_RE = re.compile(r"^hf_[A-Za-z0-9]{20,200}$")


# ---- folders --------------------------------------------------------------------

def helper_home():
    """This helper's own folder: Python, the models and the token live here."""
    env = os.environ.get("PINGLYMOTION_KIMODO_HOME")
    if env:
        return env
    base = os.environ.get("LOCALAPPDATA") or os.path.join(os.path.expanduser("~"), ".local", "share")
    return os.path.join(base, "PinglyMotion", "kimodo")


def documents_folder():
    if os.name == "nt":
        try:
            # FOLDERID_Documents, also when it was moved (for example to OneDrive)
            class GUID(ctypes.Structure):
                _fields_ = [("a", ctypes.c_uint32), ("b", ctypes.c_uint16), ("c", ctypes.c_uint16),
                            ("d", ctypes.c_ubyte * 8)]
            fid = GUID(0xFDD39AD0, 0x238F, 0x46AF, (ctypes.c_ubyte * 8)(0xAD, 0xB4, 0x6C, 0x85, 0x48, 0x03, 0x69, 0xC7))
            out = ctypes.c_wchar_p()
            if ctypes.windll.shell32.SHGetKnownFolderPath(ctypes.byref(fid), 0, None, ctypes.byref(out)) == 0:
                path = out.value
                ctypes.windll.ole32.CoTaskMemFree(out)
                if path:
                    return path
        except (AttributeError, OSError):
            pass
    return os.path.join(os.path.expanduser("~"), "Documents")


def work_folder():
    """The PinglyMotion work folder, the same as the program's remote control uses."""
    env = os.environ.get("PINGLYMOTION_CONTROL_FOLDER")
    if env:
        return env
    return os.path.join(documents_folder(), "PinglyMotion")


def token_path():
    return os.path.join(helper_home(), "hf_token")


def venv_python():
    home = helper_home()
    if os.name == "nt":
        return os.path.join(home, "venv", "Scripts", "python.exe")
    return os.path.join(home, "venv", "bin", "python")


# ---- small pure helpers (tested in CI) ----------------------------------------

def parse_duration(text):
    """'4s', '4', '4.5 s', '4,5s' -> seconds as float; ValueError with a plain message."""
    t = str(text).strip().lower().replace(",", ".")
    for suffix in ("seconds", "second", "sec", "s"):
        if t.endswith(suffix):
            t = t[: -len(suffix)].strip()
            break
    try:
        seconds = float(t)
    except ValueError:
        raise ValueError("The length must be a number of seconds, for example 4s.")
    if not (seconds == seconds) or seconds < MIN_SECONDS or seconds > MAX_SECONDS:
        raise ValueError("The length must be between %g and %g seconds." % (MIN_SECONDS, MAX_SECONDS))
    return round(seconds, 2)


def clean_prompt(text):
    """Kimodo works best with prompts that start with 'A person'."""
    t = " ".join(str(text).split())
    if not t:
        raise ValueError("Write what the avatar should do, for example: wave with the right hand.")
    if len(t) > MAX_PROMPT:
        raise ValueError("The text is too long; keep it to one or two actions (at most %d letters)." % MAX_PROMPT)
    low = t.lower()
    if not (low.startswith("a ") or low.startswith("an ") or low.startswith("the ") or low.startswith("person ")):
        t = "A person " + t[0].lower() + t[1:]
    if t[-1] not in ".!?":
        t += "."
    return t


def slug(text, limit=40):
    words = re.findall(r"[a-z0-9]+", str(text).lower())
    drop = {"a", "an", "the", "person"}
    words = [w for w in words if w not in drop] or ["motion"]
    out = ""
    for w in words:
        nxt = (out + "_" + w) if out else w
        if len(nxt) > limit:
            break
        out = nxt
    return out or words[0][:limit]


def output_path(folder, prompt, name=None):
    """A new file name in the work folder that does not replace another file."""
    base = slug(name) if name else "kimodo_" + slug(prompt)
    path = os.path.join(folder, base + ".bvh")
    n = 2
    while os.path.exists(path):
        path = os.path.join(folder, "%s_%d.bvh" % (base, n))
        n += 1
    return path


def valid_token(token):
    return bool(TOKEN_RE.match(token or ""))


def redact(text, token):
    if token and text:
        text = text.replace(token, "hf_***")
    # any other token-like text too (a token from an older login, for example)
    return re.sub(r"hf_[A-Za-z0-9]{20,}", "hf_***", text or "")


def parse_nvidia_smi(output):
    """Lines of 'name, memory.total [MiB], driver_version' -> list of dicts."""
    gpus = []
    for line in (output or "").splitlines():
        parts = [p.strip() for p in line.split(",")]
        if len(parts) < 3:
            continue
        try:
            memory = int(float(parts[1]))
        except ValueError:
            continue
        gpus.append({"name": parts[0], "memory_mb": memory, "driver": parts[2]})
    return gpus


def driver_major(version):
    try:
        return int(str(version).split(".")[0])
    except ValueError:
        return 0


# ---- token storage --------------------------------------------------------------

def read_token():
    try:
        with open(token_path(), "r", encoding="ascii") as f:
            token = f.read().strip()
    except (OSError, UnicodeDecodeError):
        return None
    return token if valid_token(token) else None


def save_token(token):
    os.makedirs(helper_home(), exist_ok=True)
    path = token_path()
    tmp = path + ".new"
    fd = os.open(tmp, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
    with os.fdopen(fd, "w", encoding="ascii") as f:
        f.write(token)
    os.replace(tmp, path)


def forget_token():
    try:
        os.remove(token_path())
        return True
    except FileNotFoundError:
        return False


def check_access(token):
    """'ok', 'bad-token', 'no-licence' or 'offline' for the gated Llama 3 files."""
    req = urllib.request.Request(GATED_PROBE, method="HEAD",
                                 headers={"Authorization": "Bearer " + token, "User-Agent": "pinglymotion-kimodo-helper"})
    try:
        with urllib.request.urlopen(req, timeout=20) as r:
            return "ok" if r.status < 400 else "offline"
    except urllib.error.HTTPError as e:
        if e.code == 401:
            return "bad-token"
        if e.code == 403:
            return "no-licence"
        return "offline"
    except (urllib.error.URLError, OSError):
        return "offline"


# ---- the machine ----------------------------------------------------------------

def ram_gb():
    if os.name == "nt":
        class MEMORYSTATUSEX(ctypes.Structure):
            _fields_ = [("dwLength", ctypes.c_ulong), ("dwMemoryLoad", ctypes.c_ulong),
                        ("ullTotalPhys", ctypes.c_ulonglong), ("ullAvailPhys", ctypes.c_ulonglong),
                        ("ullTotalPageFile", ctypes.c_ulonglong), ("ullAvailPageFile", ctypes.c_ulonglong),
                        ("ullTotalVirtual", ctypes.c_ulonglong), ("ullAvailVirtual", ctypes.c_ulonglong),
                        ("ullAvailExtendedVirtual", ctypes.c_ulonglong)]
        status = MEMORYSTATUSEX()
        status.dwLength = ctypes.sizeof(MEMORYSTATUSEX)
        if ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(status)):
            return status.ullTotalPhys / 1024 ** 3
        return None
    try:
        with open("/proc/meminfo") as f:
            for line in f:
                if line.startswith("MemTotal:"):
                    return int(line.split()[1]) / 1024 ** 2
    except OSError:
        pass
    return None


def run_check(out=print):
    """Plain-words check of the computer. True when it can run Kimodo."""
    ok = True
    smi = shutil.which("nvidia-smi")
    gpus = []
    if smi:
        try:
            r = subprocess.run([smi, "--query-gpu=name,memory.total,driver_version", "--format=csv,noheader,nounits"],
                               capture_output=True, text=True, timeout=30)
            gpus = parse_nvidia_smi(r.stdout)
        except (OSError, subprocess.SubprocessError):
            gpus = []
    if not gpus:
        out("Graphics card: no NVIDIA card found (nvidia-smi did not answer).")
        out("  Kimodo needs an NVIDIA graphics card with its driver installed.")
        ok = False
    else:
        best = max(gpus, key=lambda g: g["memory_mb"])
        out("Graphics card: %s, %.1f GB, driver %s." % (best["name"], best["memory_mb"] / 1024, best["driver"]))
        if best["memory_mb"] < MIN_VRAM_MB:
            out("  Too little graphics memory: at least %.0f GB is needed." % (MIN_VRAM_MB / 1024))
            ok = False
        if driver_major(best["driver"]) < MIN_DRIVER:
            out("  The driver is too old. Update it from nvidia.com (GeForce Game Ready or Studio driver), then run this again.")
            ok = False
    ram = ram_gb()
    if ram is None:
        out("Memory: could not be read.")
    else:
        out("Memory: %.0f GB." % ram)
        if ram < MIN_RAM_GB:
            out("  Too little memory: the text reader needs about 16 GB on its own; at least %d GB is needed." % MIN_RAM_GB)
            ok = False
        elif ram < GOOD_RAM_GB:
            out("  Enough, but close other programs (also the viewer) while a motion is made.")
    os.makedirs(helper_home(), exist_ok=True)
    free = shutil.disk_usage(helper_home()).free / 1024 ** 3
    out("Free disk space for the helper: %.0f GB." % free)
    if free < MIN_DISK_GB and not os.path.exists(venv_python()):
        out("  At least %d GB is needed for the downloads." % MIN_DISK_GB)
        ok = False
    out("Result: " + ("this computer can run Kimodo." if ok else "this computer cannot run Kimodo as it is."))
    return ok


# ---- login ------------------------------------------------------------------------

LICENCE_TEXT = """\
Kimodo reads your sentence with Meta's Llama 3 language model. Meta lets
people download it only after they accept Meta's licence, on the Hugging Face
website, with a free account. This is a one-time step:

  1. A web page opens: {page}
     Log in to Hugging Face (or make a free account).
  2. Read Meta's licence on that page. If you agree, fill in the form and press
     the button to accept it. Access is usually given within minutes; you get
     an e-mail from Hugging Face.
  3. Then a second page opens, where you make an access token: give it any
     name, keep the type "Read", press "Create token", and copy it.
  4. Paste the token here. It is not shown while you paste. It is kept only
     on this computer, in this helper's own folder, and never shown or sent
     anywhere but to Hugging Face for the downloads.
"""


def run_login(out=print, ask=getpass.getpass, open_page=webbrowser.open, probe=check_access):
    out(LICENCE_TEXT.format(page=LLAMA_PAGE))
    try:
        input("Press Enter to open the licence page... ")
    except EOFError:
        pass
    open_page(LLAMA_PAGE)
    try:
        input("When you have accepted the licence, press Enter to open the token page... ")
    except EOFError:
        pass
    open_page(TOKEN_PAGE)
    for _ in range(3):
        token = (ask("Paste your Hugging Face token (it is not shown) and press Enter: ") or "").strip()
        if not valid_token(token):
            out("That does not look like a Hugging Face token (it starts with hf_). Try again.")
            continue
        result = probe(token)
        if result == "bad-token":
            out("Hugging Face does not know this token. Copy it again from the token page.")
            continue
        if result == "no-licence":
            out("The token works, but Meta's licence is not accepted for this account yet (or the")
            out("access is still waiting). Accept it on the licence page, wait for the e-mail, then")
            out("run this again. The token is saved, so you do not have to paste it again.")
            save_token(token)
            return False
        save_token(token)
        if result == "offline":
            out("The token is saved. Hugging Face could not be reached to check it; it is checked again later.")
        else:
            out("The token is saved and Meta's licence is accepted for your account.")
        return True
    out("No token saved.")
    return False


# ---- running Kimodo -------------------------------------------------------------------

def kimodo_env(token):
    env = dict(os.environ)
    home = helper_home()
    env.update({
        "HF_TOKEN": token or "",
        "HF_HOME": os.path.join(home, "huggingface"),
        "HF_HUB_DISABLE_TELEMETRY": "1",
        "TEXT_ENCODER_DEVICE": "cpu",  # low-VRAM mode: under 3 GB on the graphics card
        "TEXT_ENCODER_MODE": "local",  # no text encoder server
        # Kimodo loads the model from <CHECKPOINT_DIR>/<model name>, the copy at the pinned revision
        "CHECKPOINT_DIR": models_folder(),
        "PYTHONIOENCODING": "utf-8",
    })
    return env


def models_folder():
    return os.path.join(helper_home(), "models")


def model_marker():
    return os.path.join(models_folder(), MODEL, ".pinglymotion-revision")


def model_ready():
    try:
        with open(model_marker(), encoding="ascii") as f:
            return f.read().strip() == MODEL_REVISION
    except OSError:
        return False


def model_download_command():
    """Downloads the model at the pinned revision into models/<MODEL> (huggingface_hub comes with Kimodo)."""
    code = ("import sys; from huggingface_hub import snapshot_download; "
            "snapshot_download(repo_id=sys.argv[1], revision=sys.argv[2], local_dir=sys.argv[3])")
    return [venv_python(), "-c", code, MODEL_REPO, MODEL_REVISION, os.path.join(models_folder(), MODEL)]


def ensure_model(token, out=print):
    """The Kimodo model at MODEL_REVISION; downloads it once. False if that failed."""
    if model_ready():
        return True
    if not re.match(r"^[0-9a-f]{40}$", MODEL_REVISION):
        out("This helper has no pinned model version. Get a newer PinglyMotion helper.")
        return False
    os.makedirs(models_folder(), exist_ok=True)
    code = run_streamed(model_download_command(), kimodo_env(token), token, out)
    if code != 0:
        out("Downloading the model failed (exit code %d). The lines above say why; try again." % code)
        return False
    os.makedirs(os.path.dirname(model_marker()), exist_ok=True)
    with open(model_marker(), "w", encoding="ascii") as f:
        f.write(MODEL_REVISION + "\n")
    return True


def generator_command():
    """The command that runs Kimodo's generate script (a test can point it elsewhere)."""
    override = os.environ.get("PINGLYMOTION_KIMODO_GENERATOR")
    if override:
        return [sys.executable, override]
    return [venv_python(), "-m", "kimodo.scripts.generate"]


def build_generate_args(prompt, seconds, stem, seed=None, constraints=None):
    args = [prompt, "--model", MODEL, "--duration", "%g" % seconds, "--output", stem,
            "--bvh", "--bvh_standard_tpose", "--num_samples", "1"]
    if seed is not None:
        args += ["--seed", str(int(seed))]
    if constraints:
        args += ["--constraints", constraints]
    return args


def run_streamed(cmd, env, token, out=print):
    """Run a command and pass its output on line by line, with the token blanked out."""
    try:
        p = subprocess.Popen(cmd, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                             text=True, encoding="utf-8", errors="replace")
    except OSError as e:
        out(redact("Could not start: %s" % e, token))
        return 1
    for line in p.stdout:
        out(redact(line.rstrip("\n"), token))
    return p.wait()


def run_make(prompt, length="4s", name=None, seed=None, constraints=None, out=print):
    try:
        seconds = parse_duration(length)
        text = clean_prompt(prompt)
    except ValueError as e:
        out(str(e))
        return 2
    if constraints and not os.path.isfile(constraints):
        out("The constraints file was not found.")
        return 2
    token = read_token()
    if not token:
        out("No Hugging Face token yet. Run the setup first (it asks for it).")
        return 3
    if not os.environ.get("PINGLYMOTION_KIMODO_GENERATOR") and not os.path.exists(venv_python()):
        out("Kimodo is not installed yet. Run the setup first.")
        return 3
    folder = work_folder()
    os.makedirs(folder, exist_ok=True)
    target = output_path(folder, text, name)
    out('Making "%s", %g seconds. The first time this downloads the models and can take a long' % (text, seconds))
    out("time; later it takes about a minute. The text is read on the processor, so the computer is busy.")
    # a test stands in for Kimodo and needs no model
    if not os.environ.get("PINGLYMOTION_KIMODO_GENERATOR") and not ensure_model(token, out):
        return 4
    with tempfile.TemporaryDirectory(prefix="kimodo-") as tmp:
        stem = os.path.join(tmp, "motion")
        code = run_streamed(generator_command() + build_generate_args(text, seconds, stem, seed, constraints),
                            kimodo_env(token), token, out)
        made = stem + ".bvh"
        if code != 0 or not os.path.isfile(made):
            out("Kimodo did not make a motion (exit code %d). The lines above say why." % code)
            return 4
        shutil.move(made, target)
    out("Done: %s" % target)
    out("In PinglyMotion: File > Import BVH from another skeleton, then pick this file.")
    return 0


def run_install(out=print):
    uv = os.environ.get("PINGLYMOTION_KIMODO_UV")
    if not uv or not os.path.exists(uv):
        out("Start the setup with PinglyMotion-Kimodo.bat; it brings the installer (uv).")
        return 3
    if not os.path.isfile(LOCK_FILE):
        out("The list of checked packages (requirements.lock.txt) is missing next to this helper. Get the helper again.")
        return 3
    steps = [("PyTorch for NVIDIA cards and Kimodo " + KIMODO_COMMIT[:7] + " (about 3 GB)", install_command(uv))]
    env = dict(os.environ)
    for title, cmd in steps:
        out("Installing %s..." % title)
        code = run_streamed(cmd, env, read_token(), out)
        if code != 0:
            out("Installing %s failed (exit code %d). The lines above say why; run the setup again to retry." % (title, code))
            return 4
    check = [py, "-c", "import torch; print('CUDA works' if torch.cuda.is_available() else 'NO CUDA')"]
    r = subprocess.run(check, capture_output=True, text=True)
    if "CUDA works" not in r.stdout:
        out("PyTorch is installed but cannot use the graphics card. Update the NVIDIA driver, then run the setup again.")
        return 4
    out("Installed. The models (about 17 GB) are downloaded the first time you make a motion.")
    with open(os.path.join(helper_home(), "installed.txt"), "w", encoding="ascii") as f:
        f.write("%s\n%s\n%s\n%s\n" % (MODEL, KIMODO_COMMIT, TORCH_PACKAGE, lock_digest()))
    return 0


def install_command(uv):
    """Installs exactly the packages in the lock; uv refuses any file whose SHA-256 differs."""
    return [uv, "pip", "install", "--python", venv_python(), "--require-hashes", "-r", LOCK_FILE,
            "--index-url", PYPI_INDEX, "--extra-index-url", TORCH_INDEX, "--index-strategy", "unsafe-best-match"]


def lock_digest():
    try:
        with open(LOCK_FILE, "rb") as f:
            return hashlib.sha256(f.read()).hexdigest()
    except OSError:
        return "none"


def installed():
    try:
        with open(os.path.join(helper_home(), "installed.txt"), encoding="ascii") as f:
            lines = f.read().split()
        return lines[:4] == [MODEL, KIMODO_COMMIT, TORCH_PACKAGE, lock_digest()]
    except OSError:
        return False


def run_menu(out=print):
    if not read_token():
        if not run_check(out):
            return 3
        if not run_login(out):
            return 3
    if not installed():
        code = run_install(out)
        if code:
            return code
    out("")
    out("Describe a motion in English, for example: wave with the right hand")
    while True:
        try:
            text = input("Motion (empty to stop): ").strip()
            if not text:
                return 0
            length = input("Length in seconds [4]: ").strip() or "4"
        except EOFError:
            return 0
        run_make(text, length, out=out)


# ---- command line ---------------------------------------------------------------------

def build_parser():
    p = argparse.ArgumentParser(prog="kimodo_helper", description="PinglyMotion helper for NVIDIA Kimodo (text-to-motion).")
    sub = p.add_subparsers(dest="command")
    sub.add_parser("check", help="check the graphics card, driver, memory and disk")
    sub.add_parser("login", help="accept Meta's Llama 3 licence and store your Hugging Face token")
    sub.add_parser("install", help="install PyTorch and Kimodo")
    m = sub.add_parser("make", help='make a motion: make "wave with the right hand" 4s')
    m.add_argument("text", help="what the avatar does, in English")
    m.add_argument("length", nargs="?", default="4s", help="length in seconds, 1 to 10 (default 4s)")
    m.add_argument("--name", help="file name in the work folder (without .bvh)")
    m.add_argument("--seed", type=int, help="the same seed gives the same motion again")
    m.add_argument("--constraints", help="a constraints file saved from Kimodo's own demo")
    sub.add_parser("menu", help="ask what to make, again and again (the default)")
    sub.add_parser("forget-token", help="delete the stored Hugging Face token")
    sub.add_parser("where", help="show the folders this helper uses")
    return p


def main(argv=None, out=print):
    args = build_parser().parse_args(argv)
    cmd = args.command or "menu"
    if cmd == "check":
        return 0 if run_check(out) else 1
    if cmd == "login":
        return 0 if run_login(out) else 1
    if cmd == "install":
        return run_install(out)
    if cmd == "make":
        return run_make(args.text, args.length, args.name, args.seed, args.constraints, out)
    if cmd == "forget-token":
        out("The token was deleted." if forget_token() else "There was no stored token.")
        return 0
    if cmd == "where":
        out("Helper folder (Python, models, token): " + helper_home())
        out("Work folder (the BVH files): " + work_folder())
        return 0
    return run_menu(out)


if __name__ == "__main__":
    sys.exit(main())
