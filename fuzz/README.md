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

Not fuzzed here: `SL.lim` and the `.llm` meshes (read only from Pingmator's own data folder), and the old BVH/AVM parser in `src/bvh.cpp`. `fuzz_animfile` fuzzes the new parser that replaces it (issue #62); until the program uses it (part 2 of #62), it does not test the code that opens files today.

`fuzz/corpus/gltf/` holds the start files for `fuzz_gltf` (`good_*` must be read, `bad_*` must be refused; `tests/gltfimport` checks that and writes them again when `PINGLYMOTION_WRITE_GLTF_CORPUS` is set to a folder).

`fuzz/corpus/anim/` holds the start files for `fuzz_anim` (`good_*` must be read, `bad_*` must be refused; `tests/animimport` checks that).

`fuzz/corpus/dingle/` holds the start files for `fuzz_dingle` (`good_stable_*` are exactly what the writer produces; a test checks that).

`fuzz/corpus/bvh/` and `fuzz/corpus/avm/` hold the start files for `fuzz_animfile`. Files named `bad_*` are broken on purpose (most of them are the files from the security audit); the others are valid. The `roundtrip` test (`tests/tst_roundtrip.cpp`) checks every one of them today: good files must open, bad files must be refused with a reason. It also opens, saves and opens again every animation file that ships with Pingmator, as a safety net for the new parser.

## Run

Needs clang and its sanitizer runtime (Ubuntu: `sudo apt-get install clang libclang-rt-18-dev`).

```
CC=clang CXX=clang++ cmake -S . -B build-fuzz -DCMAKE_BUILD_TYPE=Debug -DPINGLYMOTION_FUZZ=ON
cmake --build build-fuzz --target fuzz_prop fuzz_control fuzz_animfile fuzz_dingle fuzz_anim fuzz_gltf -j 2

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
```

The first folder collects new interesting inputs; `fuzz/corpus/` holds the starting inputs and is only read.

`fuzz_control` makes a fresh animation for every input, so a crash can always be repeated with that one input. That makes it slow (about 30 inputs per second); `fuzz_prop` runs thousands per second.

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
