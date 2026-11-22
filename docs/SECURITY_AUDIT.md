# PinglyMotion – Security Audit

| | |
|---|---|
| Date | 14 November 2026 |
| Repository | the old repository of the project (now PinglyMotion) |
| Revisions | `main` @ `381bed1`, `aurora-link` @ `cc97e86` (2 commits ahead of / 2 behind `main`) |
| Also reviewed | Receiving side of the live link: `pingledingle-tech/Aurora-Viewer-Fork`, the fork's former live-link branch @ `58c4351` (`crates/aurora-viewer/src/<live-link module>/`) |
| Scope | BVH/AVM/LIM/LLM/PRP parsers, HTTP control server, MCP server, Aurora live link, GitHub Actions, dependencies |
| Code changed | None. No branches merged, nothing pushed, no releases. |

## Status on `main` (18 November 2026)

**Numbers in this report:** `#25`, `#145` and similar numbers are pull requests and issues of the **old repository**, unless written as a link. The issues that were moved have new numbers ([MIGRATION.md](MIGRATION.md)): old #143 is [#23](https://github.com/pingledingle-tech/PinglyMotion/issues/23), old #146 is [#24](https://github.com/pingledingle-tech/PinglyMotion/issues/24), old #147 is [#25](https://github.com/pingledingle-tech/PinglyMotion/issues/25) and old #62 is [#9](https://github.com/pingledingle-tech/PinglyMotion/issues/9).

Every finding below was checked again against the code and workflows on `main` @ `c3e0449` (old repository); the rows changed on 18 November in the evening were checked in this repository (PinglyMotion `main` @ `235a597`). The original report further down is unchanged; its line numbers refer to the old revisions.

**How it was checked:** ASan/UBSan Debug build (GCC 13, Qt 6.4 from Ubuntu, as in appendix A). The crafted files from appendix B were regenerated and loaded with `BVH::animRead()` in a small harness built from the same sources as `pinglymotion_tests`, and some through the program itself under Xvfb. All six test suites pass with **no** ASan or UBSan report. The control server was tested live (`--control`, a separate work folder). Workflows were read on `main`. The Aurora fork (`pingledingle-tech/Aurora-Viewer-Fork`) could **not** be read in this run; its findings are marked as not verified.

**Count (18 November; AL-03 has since been fixed, see its row):** 23 fixed, 1 partly, 5 not applicable, 0 open (29 findings). Was still open as an issue on 18 November: [#23](https://github.com/pingledingle-tech/PinglyMotion/issues/23) (the live link does not check that the program on the port is Aurora; AL-03); issue 23 was closed on 19 November 2026 after pull request 46. [#24](https://github.com/pingledingle-tech/PinglyMotion/issues/24) (how the Qt downloads are verified; P-06) has its answer below. The partly fixed finding is AL-03 (issue above); P-06 was reviewed on 18 November and is now fixed (no open issue needed, see its row). Of the five not applicable, three are in the Aurora fork and were not checked (AL-01, AL-02, P-08) and two are about code that no longer exists or cannot happen any more (P-01, P-04). The parser replacement ([#9](https://github.com/pingledingle-tech/PinglyMotion/issues/9)) is finished: reading, saving and removal of the old code are merged (#42, #43).

| ID | Status | Evidence |
|---|---|---|
| PM-01 | fixed | #25 (issue #1). `channels.bvh` refused: "CHANNELS must be a whole number from 0 to 6, found "200"". No sanitizer report. Unknown channel names are refused too. Test row "too many channels (#1)" in `tst_bento.cpp`. |
| PM-02 | fixed | #25 (issue #2). `empty.bvh` and `truncated.bvh` refused with an error ("HIERARCHY" expected / Unknown joint type), no crash, also when opened through the program. Test rows "empty file", "only HIERARCHY", "no MOTION", "empty AVM". |
| PM-03 | fixed | #25 (issue #3). `negkey.avm` refused ("A key frame number must be a whole number of at least 0"). `props.avm` and `posease.avm` load; the extra properties are skipped (`i >= numKeyframes()`), no ASan report. `Frames: 0` refused. |
| PM-04 | fixed | #25 (issue #4). `poskey.avm` refused at once ("A position key frame number … at least 0, found "-7""); `getKeyframeNumberBefore()` returns early for `frame <= 0`. |
| PM-05 | fixed | #25 (issue #5). `deep.bvh` (20 000 levels) refused in 0.2 s: "more than 256 joints, or they are nested more than 64 deep". |
| PM-06 | fixed | #25 (issue #6). `frames.bvh` refused ("Frames must be a whole number from 1 to 10000"); peak memory 46 MB. File size capped (`MAX_FILE_BYTES`). |
| PM-07 | fixed | #25 (issue #7). `Constants::pinglyMotionDataPath()` uses the program folder, never the current folder. Program started in a folder with a planted `data/SLFemale.bvh` (= `channels.bvh`) and `data/SL.lim`: the planted files were not read. Test `dataFolderIsNotTheCurrentFolder`. |
| PM-08 | fixed | #27 (issue #8). Live: `save_file`/`open_file` outside the work folder (absolute path, `../`) refused with the same text; an existing other file is not overwritten; `new_animation`/`open_file` refuse while there are unsaved changes. |
| PM-09 | fixed | #26 (issue #9). `ProxyHandler({})`, only `http://127.0.0.1:` / `localhost:` URLs. fixed in the Python server, which was later removed; the Node server has the same protections and `tests/test_mcp_node.py` checks them. |
| PM-10 | fixed | #36 (issue #10). Key checked before the body, at most 8 connections, body limit 64 KB. Live: 50 connections with 4 MB bodies and no key, memory did not grow. Appendix C header part: see below (old #145, fixed). |
| PM-11 | fixed | #31 (issue #14). Saving uses `QSaveFile`, `commit()` and stream status are checked, the error is passed on. Test `saveFailureIsReported`. (Not re-tested live: the audit sandbox runs as root, so a read-only folder does not fail.) |
| PM-12 | fixed | #32 (issue #15). `control.json` still stays after a crash (checked), but the MCP server now sends the key only after the program on the port answers a nonce with HMAC-SHA256 of the key (`/api/hello`); the file is rewritten at the next start. |
| PM-13 | fixed | #25, #33, #37 (issue #16). `ikOn[]` and `BVHNode` channel fields initialised, `Figure:` and weights range-checked before casting. No UBSan report in any test suite. |
| AL-01 | not applicable (not verified) | #78 (issue #12). PinglyMotion no longer uses the fork's former live-link module (port 17656); it uses Aurora's LiveMesh (port 17655) with a key file and HMAC challenge. Whether that module was removed from the fork, and how LiveMesh treats non-JSON or HTTP lines, could not be checked (no access to the fork in this run). |
| AL-02 | not applicable (not verified) | #78 (issue #13). Same reason as AL-01: the finding is in the fork's former live-link module. LiveMesh uses length-prefixed messages with size limits according to `docs/archive/LIVE_LINK_RESEARCH.md`, but Aurora's code was not read. |
| AL-03 | partly on 18 Nov; fixed on 19 Nov 2026 by pull request 46 (issue 23 closed, the project owner tested it): the program now checks that the server is Aurora with Aurora's own LiveMesh key | Text of 18 Nov: #78 (issue #11). PinglyMotion now waits for Aurora's `hello` and proves itself with HMAC before sending a pose. It still does not check that the server is Aurora: any program on port 17655 can answer `{"ok":true}` and get the poses → [#23](https://github.com/pingledingle-tech/PinglyMotion/issues/23). |
| SC-01 | fixed | #38 (issue #17). Every action pinned to a full commit SHA; release split into build jobs (`contents: read`) and a publish job (`contents: write`) that only downloads artifacts. Dependabot updates actions. |
| SC-02 | fixed | #58 (issue #18). Windows builds and releases use Qt 6.12.0 (`QT_VERSION` in `release.yml`). |
| SC-03 | fixed | #38 (issue #19). `persist-credentials: false` in every checkout; `SHA256SUMS` published; `actions/attest-build-provenance`. Code signing (only "consider" in the fix) is not done. |
| SC-04 | fixed | Old #23, #75, #140, and in this repository #6, #29, #31. CodeQL (`analyze`) and dependency review are required checks on pull requests; sanitizer build; fuzz targets for `.prp` files, the control API, the new file parser and the new `.dingle` ([#51](https://github.com/pingledingle-tech/PinglyMotion/pull/51)), `.anim` ([#65](https://github.com/pingledingle-tech/PinglyMotion/pull/65)) and glTF ([#69](https://github.com/pingledingle-tech/PinglyMotion/pull/69)) readers run every night, all at the same time ([#71](https://github.com/pingledingle-tech/PinglyMotion/pull/71)); the three new readers have no menu yet, so users cannot reach them; Dependabot updates the actions. Slow checks (CodeQL on the tests, fuzzing) run nightly, not on every pull request. |
| P-01 | not applicable | Commit `9d77e81` removed the unused `BvhParser`/`FileManager`. The new parser is #62; its last parts (saving through the data model, removal of the old token parser and writers) are merged ([#42](https://github.com/pingledingle-tech/PinglyMotion/pull/42), [#43](https://github.com/pingledingle-tech/PinglyMotion/pull/43)). |
| P-02 | fixed | #26. `ProxyHandler({})` turns off every proxy source, also the Windows registry proxy. Not tested on Windows. |
| P-03 | fixed | Old #144. `writeConnectionFile()` writes a temporary file with `QSaveFile`, sets owner-only permissions before the key is written, checks every result and replaces `control.json` in one step. `PINGLYMOTION_CONTROL_FILE` is still honoured on purpose (the tests and smoke tests use it). |
| P-04 | not applicable | PM-01 is fixed, so there is no overflow to exploit. |
| P-05 | fixed | #58. Qt 6.12.0 instead of 6.8.3, so the listed advisories no longer apply to the shipped DLLs. |
| P-06 | fixed (reviewed) | #38, #58, [#24](https://github.com/pingledingle-tech/PinglyMotion/issues/24). aqtinstall read at the pinned commit `076e165`. Checksums for the Qt archives (`<archive>.sha256`) and for `Updates.xml` come from `mirrors.trusted_mirrors` (`get_hash()` in `aqt/helper.py`), which is `https://download.qt.io` in the shipped `settings.ini`; the archives come from `--base` / `baseurl` (also `download.qt.io` by default, and a random fallback mirror only after a connection error). So the checksums are never taken from a fallback mirror. A failed checksum fetch for an archive raises `ChecksumDownloadFailure` and a mismatch `ArchiveChecksumError` (retried, then raised); both are `AqtException`, so the install stops with exit code 1. Skipping needs `--UNSAFE-ignore-hash` or `INSECURE_NOT_FOR_PRODUCTION_ignore_hash`; neither is used in `build.yml` or `release.yml`. Trusted source can be set with `AQT_MIRRORS_TRUSTED_MIRRORS`; the default is already the Qt main server, so no workflow change was made. Remaining limits: the checksum file itself comes over HTTPS from `download.qt.io` (trust in Qt's server and TLS); for `Updates.xml` only, a failed checksum fetch gives a warning and the file is used unchecked; `aqt/helper.py` redirect handling follows up to 10 redirects without checking the target against `trusted_mirrors`. Pinning the SHA-256 of the archives in the workflow was not done: it must be redone at every Qt update, and the project owner has not decided on it. |
| P-07 | fixed | #33, #75 (issue #21). Prop types, field counts and numbers validated (`src/propfile.cpp`); `.prp` fuzz target. |
| P-08 | not applicable (not verified) | Aurora side, in the fork; error text from Aurora is shown as plain text in PinglyMotion's status bar. Spoofing by a fake server is part of [#23](https://github.com/pingledingle-tech/PinglyMotion/issues/23). |
| P-09 | fixed | The repository is public. Ruleset "Protect main" (pull request needed, required checks, no deletion or force push) and "Protect release tags" are active; secret scanning and push protection are on (checked with the GitHub API 18 November). Two-factor sign-in on the owner's account cannot be seen from a session. Checklist: [#25](https://github.com/pingledingle-tech/PinglyMotion/issues/25). |

Appendix C (15 November follow-up):

| Item | Status | Evidence |
|---|---|---|
| BVH parser continues after a token mismatch (#1) | fixed | #25: every `expect_token()` failure stops the parser with an error (`truncated.bvh`, test rows "no MOTION", "unknown channel"). |
| Header limit bypass (#10) | fixed | Old #145. Reproduced before: a complete header of 20–120 KB with the key got `200 OK`. Now `onReadyRead()` refuses a header over 16 KB (`MAX_HEADER_BYTES`) whether or not the terminator has arrived. |
| Connection file not atomic, results unchecked (#14) | fixed | Same as P-03 (old #144). |
| `.prp` types and numbers (#21) | fixed | #33, fuzz target from #75. |
| Mesh morph vertex indices (#22) | fixed | #33: `meshIndexesAreValid()` in `src/avatarskin.cpp` checks triangle and morph indices; test `badMaxMeshIsRefused`. |

Side observation (not an audit finding): `open_file` through remote control on a file that fails the check shows a modal warning in PinglyMotion. The answer only comes after someone closes the dialog, so the AI client's request times out instead of getting the error text. No crash and no data loss.

---

## How to read this report

- **Confirmed** = reproduced in this audit (crash, sanitizer report, or observed behaviour). The evidence is given.
- **Confirmed by code review** = the faulty code path is unambiguous, but it was not run end-to-end (stated per finding).
- **Potential / unverified** = plausible risk, not reproduced. Kept in a separate section.
- Line numbers refer to the revisions above.

Severity scale used:

| Level | Meaning here |
|---|---|
| Critical | Remote or zero-click compromise, or code execution reachable without user action |
| High | Memory corruption (write) reachable from a file the user opens, or equivalent |
| Medium | Crash/hang/resource exhaustion from untrusted input, file writes or token leakage that need one extra condition |
| Low | Needs local access or an unusual setup; limited impact; robustness defects with a security angle |

No Critical findings were found. PinglyMotion has no listening network service by default; the control server is opt-in and bound to 127.0.0.1.

## Summary

| ID | Severity | Status | Title |
|---|---|---|---|
| PM-01 | **High** | Confirmed | Heap buffer overflow (write) from `CHANNELS` count in BVH/AVM files |
| PM-02 | Medium | Confirmed | NULL pointer dereference on empty, truncated or malformed BVH/AVM |
| PM-03 | Medium | Confirmed | Out-of-bounds reads in the AVM key-frame / properties parser |
| PM-04 | Medium | Confirmed | Hang (signed overflow loop) on negative frame numbers in AVM `Positions:` |
| PM-05 | Medium | Confirmed | Stack overflow from deeply nested joints (unbounded recursion) |
| PM-06 | Medium | Confirmed | Memory exhaustion: 117-byte BVH allocates >3 GB (`Frames:` unbounded) |
| PM-07 | Medium | Confirmed | Data files (skeleton, limits, meshes) are loaded from the current working directory |
| PM-08 | Medium | Confirmed | Remote control can overwrite any `.bvh`/`.avm` file, open any path, and drop unsaved work |
| PM-09 | Medium | Confirmed | MCP server sends the bearer token through `HTTP_PROXY` |
| PM-10 | Low | Confirmed | Control server buffers up to 4 MB per connection before authentication, no connection limit |
| PM-11 | Low | Confirmed | `save_file` reports success when the file could not be written |
| PM-12 | Low | Confirmed | `control.json` stays behind after a crash |
| PM-13 | Low | Confirmed (UBSan) | Uninitialised members and undefined float→int conversions |
| AL-01 | Medium | Confirmed (isolated copy) | Aurora accepts poses from any web page (cross-protocol POST) and any local process |
| AL-02 | Medium | Confirmed (isolated copy) | Aurora `MAX_LINE` is not enforced while data keeps arriving → unbounded memory |
| AL-03 | Low | Confirmed by code review | PinglyMotion's live link trusts whatever listens on 127.0.0.1:17656 |
| SC-01 | Medium | Confirmed (CodeQL) | Third-party action pinned by tag runs in the release job with `contents: write` |
| SC-02 | Medium | Confirmed | Windows builds ship Qt 6.8.3, which has published security advisories |
| SC-03 | Low | Confirmed | Checkout token persisted during builds; no release checksums, signing or attestation |
| SC-04 | Low | Confirmed | No security tooling in CI (CodeQL, sanitizers, fuzzing, Dependabot) |

Potential/unverified items are listed in [section 6](#6-potential-risks-and-unverified-concerns).

---

## 1. Tools and what they found

| Tool | Target | Result |
|---|---|---|
| AddressSanitizer + UndefinedBehaviorSanitizer (GCC 13, `-fsanitize=address,undefined`, Debug) | `main` and `aurora-link`, existing test suites + crafted files | Existing suites pass (main: 43 + 29 tests; aurora-link: 7 + 43 + 27). UBSan reports uninitialised loads in normal runs (PM-13). Crafted files: heap-buffer-overflow WRITE, SEGV, stack-overflow, Qt index asserts (PM-01…PM-05, PM-07). |
| Release build (GCC, `-O2`, no sanitizer) under Xvfb | crafted files | Segfaults for PM-01, PM-02, PM-03 (`negkey.avm`), PM-05; hang for PM-04; >3 GB RSS for PM-06. Some OOB reads (PM-03 `props.avm`, `posease.avm`) do not crash in release: they read out of bounds silently. |
| CodeQL 2.27.4, `cpp-security-and-quality` | `main` (C++) and `aurora-link` (C++) | No security-tagged alerts. Relevant quality alerts: lossy float→int at `bvh.cpp:214` (root of PM-01), resource leak of `lastLoadedPositionNode` (`bvh.cpp:632`). CodeQL has no taint source for "tokens of a file read into a QString", so it does not see PM-01…PM-06. See [§1.1](#11-static-analysis-details). |
| CodeQL, `python-security-and-quality` | `tools/`, `tests/` | No security alerts (only unclosed files and an empty `except`). Does not model the proxy behaviour of PM-09. |
| CodeQL, `actions-security-and-quality` | `.github/workflows` | 2 alerts: unpinned third-party action (SC-01). |
| Clang Static Analyzer (scan-build 18) | `main` | No bugs reported. See [§1.1](#11-static-analysis-details). |
| Clang ASan/UBSan | — | Not run: the clang sanitizer runtime is not installed in the audit environment; GCC's sanitizers were used instead. |
| Rust (`cargo build --release`) | Isolated copy of Aurora's `serve()`/`parse()` | Reproduced AL-01 and AL-02. The full Aurora viewer was not built. |

### 1.1 Static analysis details

CodeQL results for `main` (excluding style-only queries: commented-out code, FIXME, float equality, `const` return types):

| Query | Location | Assessment |
|---|---|---|
| Lossy function result cast | `src/bvh.cpp:214` | **Relevant.** `numChannels=token().toFloat()` – untrusted float into an int, no range check. Part of PM-01/PM-13. |
| Resource not released in destructor | `src/bvh.cpp:632`, `src/animationview.cpp:50` | Memory leak of `lastLoadedPositionNode` / `bvh`. Not a security issue alone. |
| Inconsistent nullness check | `src/pinglymotion.cpp:64` | `selectedProp()` used without NULL check. Possible crash, not reachable from untrusted input as far as reviewed. |
| Futile conditional (×4) | `src/bvhparser.cpp:471-505` | All error branches in the (unused) new parser are empty – see P-01. |

aurora-link CodeQL: the same alerts as `main`, nothing new from `auroralink.cpp` or `bvhPoseText`. scan-build was run on `main` only; `aurora-link` adds ~250 lines that were reviewed by hand.

Clang Static Analyzer (scan-build 18, default checkers plus `alpha.security.ArrayBoundV2` and `optin.cplusplus.UninitializedObject`, 58 translation units of `pinglymotion`): **no bugs reported**. Like CodeQL, it cannot follow a loop bound that comes from file contents into the fixed-size arrays of PM-01, and it does not report the uninitialised members that UBSan found at run time (PM-13). This shows the limits of static analysis for this code base; the dynamic tests and fuzzing (§7) are what find these bugs.

---

## 2. Confirmed vulnerabilities – file parsers

All parser findings are reachable in four ways:

1. **File > Open / Add** – the user opens a file someone sent them (BVH files are traded freely in Second Life communities).
2. **Command line / "Open with"** – `PinglyMotion file.bvh` (`pinglymotionwindow.cpp:356-363`).
3. **Remote control** – the `open_file` command (`controlapi.cpp:463-474`), i.e. an AI client via MCP.
4. **Planted data folder** – see PM-07.

The parser in use is the legacy `BVH` class in `src/bvh.cpp`. The newer `BvhParser`/`FileManager` code is not called anywhere (see P-01).

### PM-01 – Heap buffer overflow (write) from the `CHANNELS` count — **High**

- **Files:** `src/bvh.cpp:214-241` (write), `src/bvh.cpp:300-303` and `:697-698` (later reads), `src/bvhnode.h:122-125` (arrays)
- **Explanation:** `BVHNode` has fixed arrays `channelType[6]`, `channelMin[6]`, `channelMax[6]`. `bvhReadNode()` takes the channel count straight from the file (`node->numChannels=token().toFloat();`) and loops `for(i=0;i<node->numChannels;i++)` writing `channelMin[i]`, `channelMax[i]` and `channelType[i]`. Any count above 6 writes past the arrays and past the heap object. `assignChannels()` and `bvhWriteNode()` later read the same indices.
- **Evidence:** `channels.bvh` (2.4 KB, `CHANNELS 200 Xrotation …`):
  ```
  bvh.cpp:220:25: runtime error: index 6 out of bounds for type 'double [6]'
  ==2245==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x51300032e660
  WRITE of size 8 at 0x51300032e660 thread T0
      #0 BVH::bvhReadNode() src/bvh.cpp:221
      #1 BVH::bvhRead(QString const&) src/bvh.cpp:432
      #2 BVH::animRead(...) src/bvh.cpp:648
      #3 Animation::loadBVH(...) src/animation.cpp:107
      ...
      #5 PinglyMotionWindow::fileAdd(QString const&) src/pinglymotionwindow.cpp:1301
  ```
  Release build: segmentation fault (exit 139).
- **Impact:** Heap memory corruption from a file the user opens. The written values are mostly fixed (`-10000`, `10000`, enum values chosen by the channel names), which limits attacker control; code execution was not attempted or demonstrated. Rated High, not Critical, because it needs the user to open the file.
- **Fix:**
  ```cpp
  bool ok = false;
  const int count = token().toInt(&ok);          // integer, not toFloat
  if (!ok || count < 0 || count > 6) { delete node; return fail("bad CHANNELS count"); }
  node->numChannels = count;
  ```
  Also reject unknown channel names instead of leaving `channelType[i]` unset, and add a `Q_ASSERT(numChannels <= 6)` in `BVHNode` accessors. Make the parser return an error (see "General parser fix" below).

### PM-02 – NULL pointer dereference on empty / truncated / malformed files — **Medium**

- **Files:** `src/bvh.cpp:182-194` (returns `NULL`), `:432-448` (no check), `:536-556` (AVM, same), `:657` (`removeNoSLNodes(root)`)
- **Explanation:** `bvhReadNode()` returns `NULL` for an unknown first token or end of file. `bvhRead()`/`avmRead()` pass the `NULL` root to `assignChannels()` and `setAllKeyFramesHelper()`, which dereference it. `expect_token()` failures are only logged.
- **Evidence:** an empty `.bvh` (0 bytes) or a file containing only `HIERARCHY`:
  ```
  bvh.cpp:402:34: runtime error: member call on null pointer of type 'struct BVHNode'
  ==2259==ERROR: AddressSanitizer: SEGV on unknown address 0x000000000120
      #2 BVHNode::numChildren() const src/bvhnode.cpp:71
      #3 BVH::setAllKeyFramesHelper(BVHNode*, int) const src/bvh.cpp:402
      #4 BVH::bvhRead(QString const&) src/bvh.cpp:448
  ```
  Release: exit 139. Via remote control, `open_file` on `empty.bvh` killed the running PinglyMotion (HTTP client got "Remote end closed connection without response").
- **Impact:** Crash; all unsaved animations in the session are lost. Any garbage file with a `.bvh` name triggers it.
- **Fix:** Return `nullptr` from `bvhRead`/`avmRead`/`animRead` when `root` is `NULL` or any `expect_token` fails; in `Animation::loadBVH` and `fileAdd` show an error and do not create the animation.

### PM-03 – Out-of-bounds reads in the AVM parser — **Medium**

- **Files / lines:**
  - `src/bvh.cpp:457-459` – `getCachedRotation(0)` when `Frames: 0` (empty cache)
  - `src/bvh.cpp:466-472` – key number from file only checked `key<lastLoadedNumberOfFrames`; negative keys pass → `QList::at(key)` in `src/bvhnode.cpp:349,354`
  - `src/bvh.cpp:497-502` – properties count from file used as index into `keyframeDataByIndex(i)` → `keys[index]` in `src/bvhnode.cpp:332` without bounds check
  - `src/bvh.cpp:606-614` – same for `PositionsEase:`
- **Evidence:**
  - `negkey.avm` (key `-5`): Debug `ASSERT failure in QList::at: "index out of range"`; Release: segfault (exit 139).
  - `props.avm` (50 properties for 2 key frames) and `posease.avm`: Debug `ASSERT failure in QList::operator[]: "index out of range"`; Release: no crash – the read is silently out of bounds.
- **Impact:** Crash, or reading heap memory as key-frame numbers. Release builds define `QT_NO_DEBUG`, so Qt's bounds asserts are off and these become plain OOB reads.
- **Fix:** Validate every count and index read from the file: `0 <= key < lastLoadedNumberOfFrames`, `i < root->numKeyframes()`, `Frames >= 1`. Stop with an error on violation. Prefer `value(i, default)` or explicit checks over `operator[]`/`at()` on file-controlled indices.

### PM-04 – Hang on negative frame numbers in `Positions:` — **Medium**

- **Files:** `src/bvh.cpp:596-601` → `BVHNode::frameData()` `src/bvhnode.cpp:231-240` → `getKeyframeNumberBefore()` `src/bvhnode.cpp:307`
- **Explanation:** `Positions:` key numbers are not validated. `frameData(-7)` calls `getKeyframeNumberBefore(-7)`, which runs `while(--frame && !isKeyframe(frame))` – counting down from −7 through `INT_MIN` (signed overflow, undefined behaviour) and around towards 0: roughly 4 billion map lookups.
- **Evidence:** `poskey.avm` (164 bytes, `Positions: 3 -7 999999 5`): the process was still busy after 30 s (ASan) and after 15 s (Release) and had to be killed with SIGKILL; it did not react to SIGTERM. Last log line: `Reading position frame -7`.
- **Impact:** Application hang / 100 % CPU from a tiny file. Through `open_file` it also blocks the control server (same thread).
- **Fix:** Reject keys `< 0` or `>= lastLoadedNumberOfFrames` in all AVM sections; in `getKeyframeNumberBefore()` return early for `frame <= 0`, and use `keyframes.lowerBound(frame)` instead of a linear count-down.

### PM-05 – Stack overflow from deeply nested joints — **Medium**

- **Files:** `src/bvh.cpp:251-258` (`bvhReadNode` recursion). Also recursive on the same tree: `removeNoSLNodes` (`:263-290`), `assignChannels` (`:292-319`), `setAllKeyFramesHelper` (`:389-404`), `findLoadedNode`, `copySkeletonNode`, `bvhDelete`.
- **Explanation:** Every `JOINT` recurses with no depth limit.
- **Evidence:** 20 000 nested joints (1.3 MB) → `AddressSanitizer: stack-overflow … BVH::bvhReadNode() src/bvh.cpp:254`. Release build: 300 000 levels (19.8 MB) → segfault (exit 139).
- **Impact:** Crash.
- **Fix:** Pass a depth counter and reject depth > 64 and more than ~256 joints (the SL skeleton has 133). The Aurora importer already does exactly this (`if depth > 64 || joints.len() >= 256`).

### PM-06 – Memory exhaustion from `Frames:` — **Medium**

- **Files:** `src/bvh.cpp:436-446` and `:540-554` (frame count from file, unbounded), `:297-298` (two `new QVector3D` per node per frame), `:158-163` (missing tokens silently become empty strings → `0.0`), `:424` (`readAll()` without size limit)
- **Explanation:** The frame count is trusted. Because `token()` returns an empty string after the end of the file, the motion data does not have to be present: a short header alone makes the parser allocate for every declared frame.
- **Evidence:** `frames.bvh` is 117 bytes (`… MOTION Frames: 20000000 Frame Time: 0.1`). Release build RSS: 1.93 GB after 5 s, 2.35 GB after 10 s, 2.87 GB after 20 s, 3.24 GB after 40 s, still growing (killed).
- **Impact:** The machine runs out of memory; on Windows this can freeze the desktop before the process is killed.
- **Fix:** Cap frames (the control API already uses `MAX_FRAMES = 10000`; SL's limit is 60 s), cap `frames × channels`, require that the number of motion values actually present matches before allocating, and cap the file size (e.g. 64 MB, as Aurora does). Store frames in a `QVector<QVector3D>` instead of one heap allocation per value.

### PM-07 – Data files are loaded from the current working directory — **Medium**

- **Files:** `src/constants.cpp:122-136` (returns `"./data"` on Windows/Linux when `PINGLYMOTION_DATAPATH` is not set – the case for the CI/release builds), used by `bvh.cpp:56`, `animation.cpp:103-107`, the `.llm` loader, and `parseLimFile` (`bvh.cpp:339-384`)
- **Explanation:** At start-up PinglyMotion reads `SLFemale.bvh`, `SLMale.bvh`, `TPose.avm`, `SL.lim`, `bento_joints.txt` and `character/*.llm` from `./data` relative to the **current directory**, not the program folder. Whoever controls the current directory controls these files, and every parser bug above applies to them. `parseLimFile` has its own unchecked indices (`parameters[1]` … `parameters[10]`, `bvh.cpp:360-373`).
- **Evidence:**
  - Folder with `data/SLFemale.bvh` = `channels.bvh`, PinglyMotion started there with no arguments → `heap-buffer-overflow WRITE … bvh.cpp:221` (same as PM-01).
  - Folder with `data/SL.lim` containing the single line `hip` → `ASSERT failure in QList::operator[]: "index out of range"` (Release: OOB read).
- **Impact:** A ZIP with a `.bvh` and a `data/` folder, extracted and opened from that folder (e.g. a shortcut or "Open with" whose working directory is the extracted folder), runs attacker-controlled data through the parsers. Also replaces the reference skeleton and joint limits silently. This is the same root cause as the known functional bug "BVH File not found: ./data/SLMale.bvh".
- **Fix:** Resolve data relative to `QCoreApplication::applicationDirPath()` (as already done on macOS), never the current directory. Optionally verify bundled data files against SHA-256 hashes generated at build time.

---

## 3. Confirmed vulnerabilities – HTTP control server and MCP

What is done well (verified): server binds only to `127.0.0.1` (`controlserver.cpp:78`); off by default and visible in the title bar; 256-bit token from `QRandomGenerator::system()` (`:28-37`), regenerated each start; constant-time comparison (`:40-46`); `Host` header check blocks DNS rebinding (`:233-235`); any `Origin` header is refused (`:236-237`) – tested: wrong token → 401, `Origin` → 403; request header (16 KB) and body (4 MB) limits; 15 s connection timeout; `control.json` is created with mode 0600 (verified on Linux); numeric command inputs are range-checked; JSON parsing uses Qt's parser.

### PM-08 – Remote control can overwrite any `.bvh`/`.avm` file, open any path, and drop unsaved work — **Medium**

- **Files:** `src/controlapi.cpp:463-474` (`open_file`), `:469` (existence oracle), `:476-489` (`save_file`); `src/pinglymotionwindow.cpp:111-125` (`saveFile` host), `:97-109` + `:173-176` (`newAnimation`/`openFile` call `discardChanges()`)
- **Explanation:** The only checks are the file extension and, for saving, that the folder exists. `save_file` overwrites existing files without asking. `open_file` accepts any absolute path and answers "No such file: <path>" for paths that do not exist. `new_animation` and `open_file` clear the dirty flag of every open animation, so unsaved work is discarded without the usual question. Every token holder can do this; the intended token holder is an AI agent via MCP, which can be steered by text it reads elsewhere (prompt injection).
- **Evidence:**
  ```
  save_file {"path": "/tmp/victim_dir/important.bvh"}   -> {"ok": true}
  file before: "USER DATA THAT SHOULD NOT BE LOST"
  file after:  "HIERARCHY ROOT hip { OFFSET 0.000000 0. …"
  open_file {"path": "/etc/passwd.bvh"}                 -> "No such file: /etc/passwd.bvh"
  open_file {"path": ".../empty.bvh"}                   -> PinglyMotion process crashed (PM-02)
  ```
- **Impact:** Data loss (any `.bvh`/`.avm` the user owns, plus unsaved work); the control API becomes a way to feed malicious files into the vulnerable parsers. Content written is always a valid animation, so this is not arbitrary-content write.
- **Fix:**
  - Restrict `open_file`/`save_file` to one working folder (e.g. `Documents/PinglyMotion`, configurable), resolve with `QFileInfo::canonicalFilePath()` and reject anything outside it.
  - Refuse to overwrite an existing file unless it is the currently open file, or require `"overwrite": true` and show a confirmation in the GUI.
  - `new_animation`/`open_file`: fail with "unsaved changes" when `dirty()` is true, unless the user allowed it.
  - Return the same error text for "missing" and "not allowed".

### PM-09 – MCP server sends the bearer token through `HTTP_PROXY` — **Medium**

- **File:** `tools/mcp/pinglymotion_mcp.py:52-71` (the Python server, removed since; the Node server replaced it)
- **Explanation:** `urllib.request.urlopen()` honours `HTTP_PROXY`/`http_proxy`. `proxy_bypass("127.0.0.1")` is false unless `NO_PROXY` lists it. The request to `http://127.0.0.1:<port>/api` – including `Authorization: Bearer <token>` – is then sent to the proxy. The project notes already record the symptom ("urllib needs `ProxyHandler({})` in the cloud sandbox (proxy otherwise gives 404)").
- **Evidence:** A fake proxy on port 3999 and `HTTP_PROXY=http://127.0.0.1:3999`, then `pinglymotion_mcp.request('/api', {'command':'state'})`:
  ```
  PROXY SAW: POST http://127.0.0.1:17700/api HTTP/1.1
  PROXY SAW: Authorization: Bearer 4ec3e6a457dee6269731e9ed05937bcf31bb92ebfbec02b5d3ee407ba4030311
  ```
- **Impact:** On machines with a configured HTTP proxy (corporate networks, some VPN/security tools, developer environments) the token and all poses/screenshots go to the proxy. The proxy cannot reach 127.0.0.1 on the user's machine in most setups, so direct abuse needs a proxy on the same host; leaking the secret is the issue.
- **Fix:** `opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))` and use `opener.open(...)`; also refuse `info["url"]` unless it starts with `http://127.0.0.1:` or `http://localhost:`.

### PM-10 – Pre-authentication memory use, no connection limit — **Low**

- **Files:** `src/controlserver.cpp:24-26`, `:150-163`, `:211-219` (body is buffered until complete; authorization is checked only in `handle()`, `:239-241`)
- **Evidence:** 100 local connections, each sending headers with `Content-Length: 4194304` and 4 MB − 1 byte of body, no token: RSS 179 MB → 598 MB; back to 202 MB after the 15 s timeout.
- **Impact:** Any local process – including processes of other users on a shared machine, since 127.0.0.1 is shared – can make PinglyMotion use several GB while remote control is on. Temporary.
- **Fix:** Check `Host`, `Origin` and `Authorization` as soon as the header block is complete, before reading the body; cap simultaneous connections (e.g. 8); lower `MAX_BODY_BYTES` to what commands need (64 KB).

### PM-11 – `save_file` reports success when nothing was written — **Low**

- **Files:** `src/bvh.cpp:738-739` and `:836-837` (`f.open(QFile::WriteOnly)` result ignored); `src/pinglymotionwindow.cpp:124-125` (always `return true`)
- **Evidence:** `save_file {"path": "/proc/self/nope.bvh"}` → `{"ok": true}`; no file exists. The GUI save path has the same flaw.
- **Impact:** The user or AI believes work is saved when it is not.
- **Fix:** Check `open()`, `QTextStream::status()` and use `QSaveFile` (atomic write + `commit()` result); propagate the error.

### PM-12 – `control.json` stays behind after a crash — **Low**

- **Files:** `src/pinglymotionwindow.cpp:350` (removed only on `aboutToQuit`), `src/controlserver.cpp:123-143`
- **Evidence:** After the PM-02 crash via `open_file`, `/tmp/ctl.json` (token, port, PID) was still present.
- **Impact:** The token is useless once PinglyMotion is gone. But the MCP server keeps using the file: another local program that binds the same port gets the token and can answer with its own tool descriptions (prompt injection into the AI).
- **Fix:** In the MCP server, check that the `pid` in `control.json` is a running PinglyMotion before sending; in PinglyMotion, overwrite/remove a stale file at start.

### PM-13 – Uninitialised members and undefined float→int conversions — **Low**

- **Files / evidence (UBSan in ordinary test runs, no crafted input):**
  - `src/animation.h:185` `bool ikOn[NUM_IK]` is never initialised → `animation.cpp:183, 349, 495, 594: load of value 190, which is not a valid value for type 'bool'`. IK can switch on at random.
  - `BVHNode::channelType[]`/`channelOrder` unset for End sites and unused slots → `bvh.cpp:112, 116: load of value 3200171710, which is not a valid value for type 'BVHChannelType' / 'BVHOrderType'`.
  - `bvh.cpp:214` float → int (`CHANNELS 1e10`), `bvh.cpp:577` arbitrary integer cast to `Animation::FigureType`, `avatarskin.cpp:195` `static_cast<int>(std::floor(weight))` for NaN/huge weights – undefined behaviour.
- **Fix:** Initialise `ikOn` (`std::fill` in the constructor), zero-initialise the `BVHNode` arrays, parse integers with `toInt(&ok)` and range-check enums/weights before casting.

---

## 4. Confirmed vulnerabilities – Aurora live link

Trust boundary: PinglyMotion (client) ⇄ TCP 127.0.0.1:17656 ⇄ Aurora (server, own fork). Nothing in the protocol authenticates either side. Both programs treat "loopback" as "trusted", but loopback is reachable by every local process, every local user, and – for the Aurora side – by web pages through cross-protocol requests.

Positive: Aurora's link is opt-in and off at every start, binds only to `127.0.0.1`, decodes on its own thread, and its BVH importer has strict limits (64 MB, depth ≤ 64, ≤ 256 joints, `MAX_MOTION_VALUES`, ≤ 60 s). PinglyMotion's client caps an unterminated incoming line at 1 MB (`auroralink.cpp:150-154`) and shows Aurora's error text as plain status-bar text, not rich text.

### AL-01 – Aurora accepts poses from any web page and any local process — **Medium**

- **Repository:** Aurora-Viewer-Fork, the fork's former live-link branch
- **Files:** `crates/aurora-viewer/src/<live-link module>/mod.rs:143-159` (`parse`), `:173-210` (`serve`)
- **Explanation:** No token and no required handshake: a `pose` line is accepted as the first message. Lines that are not JSON only produce an `error` reply; the connection stays open. A browser `fetch('http://127.0.0.1:17656/', {method:'POST', mode:'no-cors', body:'\n{"type":"pose","bvh":"…"}\n'})` sends HTTP header lines (rejected one by one) followed by the body line, which is accepted.
- **Evidence:** On an isolated copy of `serve()`/`parse()` (verbatim; only `decode()` replaced by a print), the exact bytes a browser sends for that request, including `Origin: https://evil.example`:
  ```
  POSE ACCEPTED (40 bytes): "HIERARCHY ROOT hip ... (from a web page)"
  ```
  Not tested in a real browser. Chromium's Local Network Access permission may block or prompt for public sites → 127.0.0.1; other browsers may not.
- **Impact:** While the link is on, any website or local process can put poses on the user's avatar (local only, nothing reaches the region) and can feed arbitrary BVH text into the importer. Low direct harm, but an unauthenticated path from the web into a parser.
- **Fix:** (1) Require `hello` as the first line, with a per-session secret: Aurora writes a random token to a 0600 file (like PinglyMotion's `control.json`), PinglyMotion reads it and sends it in `hello`; close the connection on a missing/wrong token. (2) Close the connection on the first line that is not valid JSON. (3) Reject lines starting with an HTTP method (`GET `, `POST `, `OPTIONS `…).

### AL-02 – Aurora `MAX_LINE` is not enforced while data keeps arriving — **Medium**

- **Files:** `crates/aurora-viewer/src/<live-link module>/mod.rs:184-207`
- **Explanation:** `reader.read_until(b'\n', &mut line)` only returns at a newline, EOF, or an error (read timeout of 200 ms). The `line.len() > MAX_LINE` check runs after it returns. A sender that streams without a newline and without a 200 ms pause makes `line` grow without limit.
- **Evidence:** Isolated copy, client streaming 1 MB chunks with no newline:
  ```
  sent 300 MB: server VmRSS=330 MB
  sent 600 MB: server VmRSS=631 MB
  sent 900 MB: server VmRSS=930 MB
  ```
  In a second run the check only fired at `1572864000` bytes (1.5 GB), after the client closed.
- **Impact:** Aurora (the viewer, logged into SL) can be driven out of memory by any local process, or by a web page sending a large request body (subject to AL-01's browser caveat).
- **Fix:** `reader.by_ref().take((MAX_LINE + 1 - line.len()) as u64).read_until(b'\n', &mut line)`, then check the length. Also reduce `MAX_LINE`: a Bento pose is ~20 KB, 1 MB is plenty.

### AL-03 – PinglyMotion trusts whatever listens on 127.0.0.1:17656 — **Low**

- **Repository:** PinglyMotion `aurora-link`
- **Files:** `src/auroralink.cpp:88-92` (connects, no identity check), `:94-109` (sends `hello` and the pose immediately, without waiting for Aurora's `hello`), `:157-162` (Aurora's `hello` is never checked)
- **Explanation:** Confirmed by code review (not run end-to-end). Whoever binds port 17656 first – another local user, or any program while Aurora's link is off – receives the pose stream and can put messages in PinglyMotion's status bar. On Aurora's side only one client is served at a time and an idle client is never dropped (`mod.rs:104-121`, `:185`), so a squatting client also blocks the real PinglyMotion.
- **Impact:** Pose data leak (low value), spoofed status messages, denial of the link.
- **Fix:** Same token handshake as AL-01, verified in both directions; wait for Aurora's `hello` (protocol 1, feature `pose`) before sending any pose; in Aurora, drop clients that have not sent a valid `hello` within ~2 s.

Side note (not security): `aurora-link` rebuilds the full BVH text every 50 ms (`auroralink.cpp:120-132`) only to compare it with the last one sent; a dirty flag would avoid the CPU cost.

---

## 5. Confirmed findings – GitHub Actions and supply chain

Positive: workflow-level `permissions: contents: read`; `contents: write` only on the release job; the `workflow_dispatch` input is passed through `env:` and validated with a regex (no script injection); the update checker only shows a notice and opens a fixed GitHub URL over HTTPS – it never downloads or runs anything (`releaseinfo.cpp:13-37`).

### SC-01 – Third-party action pinned by tag in the release job — **Medium**

- **Files:** `.github/workflows/release.yml:38` (`jurplel/install-qt-action@v4`) and `:28` (`actions/checkout@v4`), `:23-24` (`contents: write`); `.github/workflows/build.yml:15-16, 37`
- **Evidence:** CodeQL `actions/unpinned-tag` on `build.yml:16` and `release.yml:38`.
- **Explanation:** A tag can be moved. If `jurplel/install-qt-action` is compromised, its code runs in the job that holds a `contents: write` token and builds the binaries users download – it could publish a tampered release or push to the repository.
- **Fix:** Pin every action to a full commit SHA (with the version as a comment) and let Dependabot update them; split the release into a build job (`contents: read`) and a publish job (`contents: write`) that only downloads the artifact and runs `gh release create`.

### SC-02 – Windows builds ship Qt 6.8.3 — **Medium**

- **Files:** `.github/workflows/build.yml:18`, `release.yml:40` (`version: '6.8.3'`)
- **Explanation:** Qt 6.8.3 is affected by several published advisories, e.g. CVE-2025-5455 (`qDecodeDataUrl`, used by `QNetworkReply`, fixed in 6.8.4), CVE-2025-3512 (heap overflow in `QTextMarkdownImporter`, fixed in 6.8.4), CVE-2025-5992 (DoS in `QColorTransferGenericFunction`). Whether PinglyMotion's code paths reach these was not verified; the DLLs are shipped regardless.
- **Fix:** Move to the newest Qt 6 patch release available through aqtinstall and re-check advisories per release. On Linux CI the distro Qt is used (patched by Ubuntu).

### SC-03 – Persisted checkout token; no checksums, signing or provenance — **Low**

- **Files:** `release.yml:28`, `build.yml:15` (`persist-credentials` defaults to true), `release.yml:61-65`
- **Explanation:** The job token is stored in `.git/config` for the whole job, so every later build step (CMake, windeployqt, the Qt action) can read it. Releases have no SHA-256 file, no Authenticode signature and no build provenance, so users cannot verify a download and Windows SmartScreen will warn.
- **Fix:** `with: persist-credentials: false`; publish `SHA256SUMS`; add `actions/attest-build-provenance`; consider code signing (e.g. SignPath for open-source projects).

### SC-04 – No security tooling in CI — **Low**

- No CodeQL workflow, no sanitizer job, no fuzzing, no Dependabot (`.github/dependabot.yml` absent).
- Known maintenance items (from the project notes, not re-verified here): actions on Node 20, `ubuntu-latest` drifting to a new Ubuntu release.
- **Fix:** see §7.

Dependencies: the C++ code depends only on Qt (and the bundled `libquat`); Python tools use only the standard library; no `requirements.txt`, `package.json` or vendored binaries were found. The bundled Linden `.llm` meshes and `avatar_skeleton.xml` are data, under CC BY-SA 3.0 / LGPL as already documented.

---

## 6. Potential risks and unverified concerns

These were not reproduced. They are listed so they are not forgotten.

| ID | Area | Concern | Why unverified / next step |
|---|---|---|---|
| P-01 | `src/bvhparser.cpp:153-510`, `src/filemanager.cpp` | The newer `BvhParser` dereferences the end iterator in `nextToken()`/`token()` (`:458-467`) on truncated files, skips the first token, has unbounded recursion and channel counts, and all error branches are empty. | Dead code today (no callers of `FileManager::loadAnimationFromFile`). Fix or delete before it is wired in. |
| P-02 | `pinglymotion_mcp.py` (removed; the Node server replaced it) on Windows | Whether urllib on Windows sends 127.0.0.1 through the system (registry) proxy depends on `ProxyOverride`/`<local>` settings. | Test on the maintainer's Windows PC with a system proxy set. The PM-09 fix covers it either way. |
| P-03 | `controlserver.cpp:133-141` | `control.json` is opened (created with umask permissions), then `chmod 0600`, then written. The token is written after the chmod, so no exposure was seen. On Windows `setPermissions` does not set ACLs; protection relies on `%LOCALAPPDATA%` being private. `PINGLYMOTION_CONTROL_FILE` can redirect the file anywhere. | Low risk; use `QSaveFile` and create with restrictive permissions from the start; consider ignoring the env override in release builds. |
| P-04 | PM-01 exploitability | Whether the heap overflow can be turned into code execution. | Written values are mostly constant; not attempted. |
| P-05 | Qt CVEs (SC-02) | Whether PinglyMotion reaches the vulnerable Qt code. | PinglyMotion uses `QNetworkAccessManager` for the update check (HTTPS to api.github.com) and PNG writing; not analysed further. |
| P-06 | `jurplel/install-qt-action` / aqtinstall | Integrity of the downloaded Qt binaries (checksums from Qt's mirrors). | Not reviewed; reduce exposure via SC-01. |
| P-07 | `.prp` prop files (`pinglymotionwindow.cpp:1436-1500`) | Unbounded number of lines → many props; unknown types give a NULL mesh (not drawn). No crash found by review. | Include in fuzzing. |
| P-08 | Aurora: error messages | `decode()` error text is sent back to any client; text from `hello` (64 chars) is shown in chat. Spoofing only. | Covered by the AL-01 handshake. |
| P-09 | Repository settings | Branch protection, required reviews, who may push `v*` tags (which publish releases), 2FA. | Not visible from a clone; check in GitHub settings. |

---

## 7. Fuzz testing recommendations

All external file parsers and both network protocols should be fuzzed. Suggested targets (libFuzzer with `-fsanitize=fuzzer,address,undefined`, or AFL++):

| Target | Entry point | Notes | Seeds |
|---|---|---|---|
| BVH | `BVH::animRead()` | Refactor so the parser takes a `QByteArray` (no file path, no `QMessageBox`), then fuzz `bvhRead`/`avmRead` directly. Would have found PM-01…PM-06 within seconds. | `bin/data/*.bvh`, `tools/legacy/*.bvh`, exports from `pinglymotion_tests` |
| AVM | same, `.avm` branch | Include the `Properties`, `Positions:`, `PositionsEase:`, `Scale:`, `Figure:`, `LoopIn/Out:` sections | `examples/*.avm`, `bin/data/TPose.avm` |
| Joint limits | `BVH::parseLimFile()` | needs a root node; build one from `SLFemale.bvh` once | `bin/data/SL.lim` |
| Linden meshes | `LLMMesh::load()` + `AvatarSkin::loadSkinnedPart()` | binary format; use a structure-aware mutator or just raw bytes | `bin/data/character/*.llm` |
| Props | prop loading in `fileLoadProps()` (move into a function) | text | a few saved `.prp` |
| Bento joint list | `BVH::loadBentoJoints()` | text | `bin/data/bento_joints.txt` |
| Control API | `ControlApi::execute()` with a fake `ControlHost` | JSON; check that no input crashes or writes outside the allowed folder | requests from `tests/smoke_control.py` |
| HTTP framing | `ControlServer::completeRequestLength()` + `handle()` (factor out of the socket) | header splitting, Content-Length edge cases | captured requests |
| Aurora link (PinglyMotion side) | `AuroraLink::handleLine()` / `onReadyRead()` buffer logic | | |
| Aurora link (Aurora side) | `parse()` and `parse_bvh_pose()` with `cargo fuzz` | | PinglyMotion `bvhPoseText` output |

Process:

1. Add a `PINGLYMOTION_FUZZ=ON` CMake option building one fuzzer per target.
2. Run each target for a few minutes in CI on every push (ClusterFuzzLite or a plain job), and longer nightly.
3. Keep the corpus and every crash input in `tests/fuzz/corpus/<target>/`; add each fixed crash as a regression test in `tst_bento.cpp`.
4. Also add an ASan/UBSan CI job that runs the existing tests and `smoke_gui.sh` with the crafted files from this audit (appendix B).

---

## 8. General parser fix (applies to PM-01…PM-07)

The root cause is that the legacy parser has no error path: `token()` returns an empty string at end of file, `expect_token()` only logs, and counts/indices from the file are used directly. Recommended shape:

- Parse from a `QByteArray` with a maximum size; return `std::optional<Result>` or set an error string; on any error, free the partial tree and return.
- `bool ok` on every `toInt`/`toFloat`; reject NaN/Inf where they make no sense.
- Limits: depth ≤ 64, joints ≤ 256, channels ∈ {0, 3, 6}, frames ≤ 10 000 (or the 60 s SL limit), `frames × channels` ≤ a fixed maximum, every key/index range-checked.
- Show one clear error dialog ("This file is not a valid BVH/AVM file") instead of crashing.
- The Aurora importer (`aurora-assets/src/import_animation.rs`) already has these limits and is a good reference.

---

## Appendix A – Reproduction environment

- Ubuntu 24.04 container, 2 CPUs, 7 GB RAM; Qt 6.4.2 from Ubuntu packages; GCC 13.3; CodeQL 2.27.4; Rust stable.
- ASan/UBSan build: `cmake -S . -B /tmp/asan -DCMAKE_BUILD_TYPE=Debug -DPINGLYMOTION_TESTS=ON -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -g" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"`; run from `bin/` with `ASAN_OPTIONS=detect_leaks=0`.
- Release build: `-DCMAKE_BUILD_TYPE=Release`, run under `xvfb-run` with `QT_QPA_PLATFORM=xcb`. (With `QT_QPA_PLATFORM=offscreen` and Mesa in this container, both builds crash inside `QPlatformBackingStore::rhiFlush` on the first paint for every file, including valid ones. That is an environment issue and was excluded; all crash results above were re-checked under Xvfb.)
- CodeQL: `codeql database create --language=cpp` with a CMake build of `pinglymotion`; suites `cpp-security-and-quality`, `python-security-and-quality`, `actions-security-and-quality`.
- Aurora: `serve()`/`parse()` copied from `mod.rs` @ `58c4351` into a standalone crate (only `decode()` stubbed), driven by a Python client.

## Appendix B – Proof-of-concept inputs

Kept outside the repository. All are tiny text files and can be regenerated:

| File | Content (abridged) | Finding |
|---|---|---|
| `channels.bvh` | `HIERARCHY ROOT hip { OFFSET 0 0 0 CHANNELS 200 Xrotation ×200 } MOTION Frames: 1 Frame Time: 0.1 0 ×200` | PM-01 |
| `empty.bvh`, `truncated.bvh` | empty / `HIERARCHY` | PM-02 |
| `negkey.avm` | 2-frame AVM, key frame list `1 -5` | PM-03 |
| `props.avm` | `Properties 50 3 3 …` for 2 key frames | PM-03 |
| `posease.avm` | `PositionsEase: 40 3 3 …` | PM-03 |
| `poskey.avm` | `Positions: 3 -7 999999 5` | PM-04 |
| `deep.bvh` / `deep300k.bvh` | 20 000 / 300 000 nested `JOINT j { OFFSET 0 0 0 CHANNELS 3 … ` | PM-05 |
| `frames.bvh` | `HIERARCHY ROOT hip { OFFSET 0 0 0 CHANNELS 3 Xrotation Yrotation Zrotation } MOTION Frames: 20000000 Frame Time: 0.1` (117 bytes) | PM-06 |
| `data/SLFemale.bvh`, `data/SL.lim` in the start folder | `channels.bvh` / the line `hip` | PM-07 |


---

## Appendix C – Independent follow-up review (15 November 2026)

This addendum records an independent **static code review** performed after the original 14 November audit. It supplements rather than replaces the original test evidence. **No new sanitizer reproductions, runtime exploit demonstrations, or comprehensive dependency audits were performed for the items below.** Findings are linked to their GitHub tracking issues; issue existence does not imply a fix has been merged.

| Finding | Assessment | Tracking | Additional detail / recommended action |
|---|---|---|---|
| BVH parser continues after structural token mismatch | Confirmed by code review; not reproduced separately | old #1 | `BVH::expect_token()` reports failure, but `bvhReadNode()` does not consistently stop parsing. Propagate parse errors and discard partially constructed nodes. This extends the general parser fix in §8 and overlaps PM-01/PM-02. |
| HTTP header limit can be bypassed when the header terminator is already present | Confirmed by code review; not load-tested | old #10 | `completeRequestLength()` checks `MAX_HEADER_BYTES` only while waiting for the header terminator; `onReadyRead()` appends the complete `readAll()` result before applying limits. Enforce header and total-buffer limits before or during accumulation, including complete oversized headers. This extends PM-10. |
| Connection-file creation is not atomic and write/permission results are unchecked | Confirmed by code review; no failure-injection test | old #14 | `ControlServer::writeConnectionFile()` truncates the destination via `QFile`, without validating the full write count or the result of `setPermissions()`. Prefer `QSaveFile`, check all results, and fail closed if the token file cannot be written securely. Related to PM-11 and potential concern P-03. |
| Unvalidated `.prp` prop types and numeric fields | Potential robustness/security concern; exploitability unverified | old #21 | `PinglyMotionWindow::fileLoadProps()` converts fields without sufficient validation; unknown prop types may produce a null mesh passed to `scene->addProp()`. Validate supported types, finite numbers, field counts and ranges; test malformed files. This expands potential concern P-07. **No crash or exploitable dereference was established.** |
| Mesh morph vertex indices lack local bounds checks | Conditional concern; upstream validation not established | old #22 | `AvatarSkin` uses morph `vertexIndices` and mesh triangle indices without an obvious local bounds check. Confirm what `LLMMesh` validates, add explicit range checks at the use sites, and fuzz malformed meshes. **No out-of-bounds access or exploit was reproduced for this finding.** |

### Tracking and interpretation

- The original audit findings remain tracked in old issues #1–#20 (private archive); the independent review updated the descriptions of **#1, #10 and #14** and opened **#21 and #22**.
- The original severity classifications and evidence remain unchanged. In particular, the two new issues must **not** be presented as confirmed vulnerabilities until the relevant failure modes and input reachability are demonstrated.
- Recommended regression coverage: complete oversized HTTP headers; malformed BVH structural tokens; simulated token-file write and permission failures; malformed `.prp` types and numeric fields; invalid LLM morph indices.
- Review scope and date are distinct from the original audit. This addendum is a documentation update, **not** evidence that any vulnerability has been remediated.
