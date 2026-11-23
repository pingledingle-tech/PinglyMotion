# Fuzz tests

Fuzzing feeds random and broken input to a piece of code, millions of times, and stops when the program crashes or breaks a rule. It finds the kind of bugs the security audit found in the old BVH parser.

The targets, all built with libFuzzer, AddressSanitizer and UndefinedBehaviorSanitizer:

| Target | What it tests | Why |
|---|---|---|
| `fuzz_prop` | `PropFile::parse()` – prop files (`.prp`) | People send each other prop files |
| `fuzz_control` | `ControlApi::execute()` – the remote control commands | An AI client sends them, and it can be steered by text it read somewhere else |
| `fuzz_animfile` | `AnimFile::parse()` – the new BVH/AVM parser (`src/animfile.*`, issue #62), each input as a BVH and as an AVM file | BVH files are traded freely in Second Life communities, and they reach the parser through File > Open, "Open with" and the remote control |
| `fuzz_dingle` | `Dingle::parse()` and `Dingle::write()` – the `.dingle` working format (`src/dingle.*`, issue #26, [DINGLE_FORMAT.md](../docs/DINGLE_FORMAT.md)); an accepted file must keep every rule, and `write(parse(x))` must read back equal and stay byte-stable | `.dingle` files will be opened from outside, like BVH files |
| `fuzz_anim` | `AnimImport::parse()` – Second Life `.anim` files (`src/animimport.*`, issue #22, [ANIM_FORMAT.md](../docs/ANIM_FORMAT.md)); an accepted file must keep every limit of the data model | `.anim` files will be opened from outside |
| `fuzz_gltf` | The glTF import (`src/gltfimport.*`, issue #12): `GltfImport::read()`, sampling to frames with both rest poses, and retargeting onto a small Second Life skeleton; every accepted result must keep the rules of a parsed animation | `.glb` and `.gltf` files come from the internet (Blender, Mixamo and other tools export them) |
| `fuzz_retargetmapping` | The hand-made joint mapping files for retargeting (`src/retargetmapping.*`, issue #16 step 4, [RETARGET_MAPPING.md](../docs/RETARGET_MAPPING.md)): `RetargetMapping::parse()`, the writer round trip, the entries on top of the automatic mapping of a small skeleton, and the retarget with it | A mapping file can come from another person or be written by an AI client |
| `fuzz_openfile` | Opening a file end to end (issue 250): each input written as a `.bvh`, `.avm` and `.dingle` file, then `BVH::checkFile()` and a whole `Animation` (`BVH::animRead`, the joint tree, the limits, the Bento reference skeleton), as File > Open and the remote control do | The parsers have their own targets; this one tests what happens after them with a real file |
| `fuzz_retargetbvh` | Importing a BVH from another skeleton (issue 250): `Retarget::mapSkeletons()` on foreign joint names and trees, then `Retarget::retarget()` with both rest poses onto `bin/data/SLFemale.bvh`; mapping indices stay inside both skeletons and every result passes `Retarget::validate()` | BVH files from other programs (Mixamo, CMU, XR Animator, MoMask) are imported this way |
| `fuzz_controlserver` | The control server's HTTP framing (issue 250): any bytes over a real local connection, in two pieces, to `ControlServer` (request line, headers, limits, Host, Origin, key, Content-Length, body); `@KEY@` and `@PORT@` in an input become the real key and port | Any program on the computer can connect to the server |
| `fuzz_rlv` | The Script Lab's RLV core (`src/scriptlab/`, issue 287): each line `<object>|<text>` goes to `Rlv::parseLine()` and `RlvState::apply()`; after every line the limits hold, the counters match the held restrictions, replies stay within 1023 characters and blocked actions follow the plain restrictions | LSL scripts and AI clients send RLV lines through the control command `rlv_command` |
| `fuzz_lsl` | The Script Lab LSL interpreter (`src/scriptlab/lsl/`, issue 289): any bytes as an LSL script through the lexer, parser and checker (what `lsl_check` does) and the static checker (what `lsl_lint` does); every error and warning has a line, a column and a message, and every lint warning a known id; a script that compiles is then run with small limits, one chat and one touch (what `lsl_run` does; `llOwnerSay("@...")` goes to the RLV core) and must stay inside the limits; the LSL-to-SLua converter (`src/scriptlab/lsl/lsl2slua.*`, lane 1 job G) converts every input: only a script that compiles gives SLua text, the text and the warnings stay inside their limits, and every error and warning has a place and a message | An AI client writes the scripts and sends them over the control commands |

Not fuzzed here: `SL.lim` and the `.pmm` body meshes (read only from PinglyMotion's own data folder). The old BVH/AVM token parser is gone: the program reads files through the new parser (`AnimFile::parse`, issue #62 finished, old parser removed in pull request 43 on 18 Nov 2026), which `fuzz_animfile` fuzzes, and `fuzz_openfile` fuzzes opening a file end to end.

`fuzz/corpus/retargetmapping/` holds the start files for `fuzz_retargetmapping` (`good_*` must be read, `bad_*` must be refused; `tests/retargetmapping` checks that).

`fuzz/corpus/gltf/` holds the start files for `fuzz_gltf` (`good_*` must be read, `bad_*` must be refused; `tests/gltfimport` checks that and writes them again when `PINGLYMOTION_WRITE_GLTF_CORPUS` is set to a folder).

`fuzz/corpus/anim/` holds the start files for `fuzz_anim` (`good_*` must be read, `bad_*` must be refused; `tests/animimport` checks that).

`fuzz/corpus/dingle/` holds the start files for `fuzz_dingle` (`good_stable_*` are exactly what the writer produces; a test checks that).

`fuzz/corpus/bvh/` and `fuzz/corpus/avm/` hold the start files for `fuzz_animfile`. Files named `bad_*` are broken on purpose (most of them are the files from the security audit); the others are valid. The `roundtrip` test (`tests/tst_roundtrip.cpp`) checks every one of them today: good files must open, bad files must be refused with a reason. It also opens, saves and opens again every animation file that ships with PinglyMotion, as a safety net for the new parser.

## Run

Needs clang and its sanitizer runtime (Ubuntu: `sudo apt-get install clang libclang-rt-18-dev`).

```
CC=clang CXX=clang++ cmake -S . -B build-fuzz -DCMAKE_BUILD_TYPE=Debug -DPINGLYMOTION_FUZZ=ON
cmake --build build-fuzz --target fuzz_prop fuzz_control fuzz_animfile fuzz_dingle fuzz_anim fuzz_gltf fuzz_retargetmapping \
  fuzz_openfile fuzz_retargetbvh fuzz_controlserver fuzz_rlv fuzz_lsl -j 2

export ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 QT_QPA_PLATFORM=offscreen
mkdir -p /tmp/prop /tmp/control
build-fuzz/fuzz/fuzz_prop -max_total_time=300 -max_len=4096 /tmp/prop fuzz/corpus/prop
build-fuzz/fuzz/fuzz_control -max_total_time=300 -max_len=2048 -dict=fuzz/control.dict /tmp/control fuzz/corpus/control
mkdir -p /tmp/animfile
build-fuzz/fuzz/fuzz_animfile -max_total_time=300 -max_len=65536 -dict=fuzz/animfile.dict /tmp/animfile fuzz/corpus/bvh fuzz/corpus/avm
mkdir -p /tmp/dingle
build-fuzz/fuzz/fuzz_dingle -max_total_time=300 -max_len=65536 -dict=fuzz/dingle.dict /tmp/dingle fuzz/corpus/dingle
mkdir -p /tmp/gltf
build-fuzz/fuzz/fuzz_gltf -max_total_time=300 -max_len=65536 -dict=fuzz/gltf.dict /tmp/gltf fuzz/corpus/gltf
mkdir -p /tmp/retargetmapping
build-fuzz/fuzz/fuzz_retargetmapping -max_total_time=300 -max_len=16384 -dict=fuzz/retargetmapping.dict /tmp/retargetmapping fuzz/corpus/retargetmapping
mkdir -p /tmp/openfile /tmp/retargetbvh /tmp/controlserver
build-fuzz/fuzz/fuzz_openfile -max_total_time=300 -max_len=65536 -dict=fuzz/animfile.dict /tmp/openfile fuzz/corpus/bvh fuzz/corpus/avm fuzz/corpus/dingle
build-fuzz/fuzz/fuzz_retargetbvh -max_total_time=300 -max_len=65536 -dict=fuzz/animfile.dict /tmp/retargetbvh fuzz/corpus/bvh tests/data/xr_animator tests/retargetsmpl tests/motions/cmu
build-fuzz/fuzz/fuzz_controlserver -max_total_time=300 -max_len=2048 -dict=fuzz/controlserver.dict /tmp/controlserver fuzz/corpus/controlserver
mkdir -p /tmp/rlv
build-fuzz/fuzz/fuzz_rlv -max_total_time=300 -max_len=4096 -dict=fuzz/rlv.dict /tmp/rlv fuzz/corpus/rlv
mkdir -p /tmp/lsl
build-fuzz/fuzz/fuzz_lsl -max_total_time=300 -max_len=8192 -dict=fuzz/lsl.dict /tmp/lsl fuzz/corpus/lsl
```

The first folder collects new interesting inputs; `fuzz/corpus/` holds the starting inputs and is only read.

`fuzz_control` makes a fresh animation for every input, so a crash can always be repeated with that one input. That makes it slow (about 30 inputs per second); `fuzz_prop` runs thousands per second. `fuzz_openfile` opens three files per input and is slow too (about 20 per second). `fuzz_controlserver` keeps one animation between inputs (about 100 per second); the server code it tests keeps nothing between connections.

## In CI

The `Fuzz` workflow builds the targets once and then runs every target as its own job, all at the same time (`fuzz <target>`). The job `fuzz` sums them up: it fails when any target failed, so the check name stays `fuzz`.

Time per target:

- every night on `main` and before every release: 10 minutes (600 seconds)
- a manual run on a branch: 2 minutes (120 seconds) by default. Give the optional `seconds` input for a longer run: Actions > Fuzz > Run workflow, or `gh api -X POST repos/pingledingle-tech/PinglyMotion/actions/workflows/fuzz.yml/dispatches -f ref=<branch> -f inputs[seconds]=600`

It does not run on pull requests; after changing a parser or the control commands, start it on the branch. It is not a required check. A crash makes the run red, and the crash file is attached to the run as `fuzz-crashes-<target>`.

A new target needs a line in the matrix of `.github/workflows/fuzz.yml` (and in the build and pack steps there).

## When a crash is found

1. Download the crash file and repeat it: `build-fuzz/fuzz/fuzz_prop crash-file`.
2. Fix the bug.
3. Add the crash file to `fuzz/corpus/prop/`, `fuzz/corpus/control/`, `fuzz/corpus/bvh/` or `fuzz/corpus/avm/`, so every later run tries it first.
