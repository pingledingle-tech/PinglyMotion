# Security and privacy in PinglyMotion

This page says, in plain words, what PinglyMotion does with your data and how it is protected. It lists **only what is proven**. Every claim is marked:

- **tested**: an automatic test checks it on every change. The test is named.
- **not tested**: it was read in the code, but no test checks it. The file is named.

Checked against the code on 23 November 2026 (job L2-security, pull request 432). If the program changes, this page must change in the same pull request (`CLAUDE.md`, "Docs in the same pull request").

How to report a security problem: [SECURITY.md](../SECURITY.md). The website's privacy page is [privacy.html](privacy.html).

## 1. What goes over the internet

| Claim | Proof |
|---|---|
| The program's only own internet use is the update check. No other part of the program opens a network connection to another computer. | **not tested.** Read in the code: the only files in `src/` that use Qt's network classes are `updatechecker.cpp` (the update check), `controlserver.cpp` (AI Assistant, this PC only) and `auroralink.cpp` (Aurora link, this PC only). No test blocks the network to prove it. |
| The update check asks GitHub's public API for the newest release (`https://api.github.com/repos/pingledingle-tech/PinglyMotion/releases/latest`). It sends the program name and version (`PinglyMotion/<version>`) and asks for JSON. GitHub also sees your IP address, as with any web request. | **not tested.** `src/releaseinfo.cpp` (address), `src/updatechecker.cpp` (request). |
| It runs when the program starts, if the last successful check is more than 7 days old or there was none. After a failed check it tries again at the next start. On the first start of a new version, the What's new window is shown instead (when the `NEWS` file is next to the program, as in the downloads) and there is no check. | **not tested.** `UpdateChecker::onStartup` in `src/updatechecker.cpp`. |
| It is on by default. You turn it off with "Check for updates automatically" in Help > About. "Check now" in the same window checks once. | **not tested.** `src/updatersettings.cpp` (default on), `src/aboutdialog.ui`, `src/aboutdialog.cpp`. |
| GitHub's answer is read only for the version number. Drafts, pre-releases, odd tags and broken answers count as "no update". | **tested:** `pinglymotion_tests`, `BentoTest::updateCheckReadsGitHubRelease`. |
| An answer over 1 MB is stopped and ignored. | **not tested.** `MAX_REPLY_BYTES` in `src/updatechecker.cpp`. |
| The program sends no usage statistics, crash reports or animations anywhere. | **not tested.** Same reading of the code as the first row. |
| Links in the program's windows (for example "Download the new version from GitHub") open in your web browser only when you click them. Then the browser makes the request, not PinglyMotion. | **not tested.** Qt `<a href>` labels in `src/*.ui` and `src/aiconnectdialog.cpp`. |
| The Script Lab never goes online. `llHTTPRequest` and the other web and e-mail calls of LSL get made-up answers inside the program. | **tested** that `llHTTPRequest` gets canned answers: test `lslhttp` (`tests/tst_lslhttp.cpp`). That no socket is opened is **not tested**: read in `src/scriptlab/lsl/world.h` (no network classes in `src/scriptlab/`). |

## 2. The AI Assistant (remote control for AI apps)

The AI Assistant lets an AI app on the same PC (Claude Desktop, ChatGPT, or another) pose, play and save the animation through a small local server. The text the program will show about it is in the register, [TEACHING.md](TEACHING.md), rows `AI-01` to `AI-08` (draft).

| Claim | Proof |
|---|---|
| The AI Assistant is off when PinglyMotion starts. Only the user turns it on (the button in the status bar or the AI Assistant window). The exception is the command-line option `--control`, for scripts and tests. | Button starts off: **tested**, `pinglymotion_aiassistant_tests` (`buttonStatesAndTexts`), `pinglymotion_statusbar_tests` (`itemsSitOnPills`), `pinglymotion_control_tests` (`indicatorShowsTheState`). That the real window does not start the server without `--control`: **not tested**, read in `src/pinglymotionwindow.cpp` (`setRemoteControl(true)` is only called for `--control` and on the user's click). |
| It listens only on this PC (127.0.0.1). Nothing on the network can reach it. | The address it gives is `http://127.0.0.1:<port>`: **tested**, `pinglymotion_control_tests` (`serverListensOnLoopbackOnly`). That the socket is bound to loopback only: **not tested**, `QHostAddress::LocalHost` in `src/controlserver.cpp`. |
| Every request needs a secret key of at least 64 characters. A request without the right key is refused. | **tested:** `pinglymotion_control_tests` (`serverRefusesWithoutKey`, `serverChecksKeyBeforeBody`, `serverListensOnLoopbackOnly`). |
| There is a new key each time it is turned on. Turning it off stops the server and deletes the key file. | **not tested** as a whole. Read in `src/pinglymotionwindow.cpp` (`setRemoteControl`) and `src/controlserver.cpp` (`start` makes a new key). |
| Requests from web pages and requests with a wrong host name are refused. | **tested:** `pinglymotion_control_tests` (`serverRefusesWebPages`, `serverRefusesWrongHost`). |
| Too many connections, oversized headers and broken requests are refused. | **tested:** `pinglymotion_control_tests` (`serverLimitsConnections`, `serverRefusesLargeHeaders`, `serverRefusesBadRequests`). The request parser is also fuzzed (`fuzz_controlserver`, `fuzz_control`). |
| The key file can only be read by your own user account. | **tested on Linux only:** `pinglymotion_control_tests` (`connectionFileHasAddressAndKey`). On Windows the test skips this check, so it is **not tested** there. |
| The status bar shows that the AI Assistant is on, and the name of each connected AI app. | **tested:** `pinglymotion_aiassistant_tests` (`buttonStatesAndTexts`, `indicatorRemembersWhoConnected`), `pinglymotion_control_tests` (`indicatorShowsTheState`). Limit: the name is the one the AI app gives itself; the program cannot check it. A program without a name shows as "A program". |
| The window title also says "AI Assistant on" while it is on. | **not tested.** `updateWindowTitle` in `src/pinglymotionwindow.cpp`. |
| The program remembers only the names of AI apps that connected and when, never the key. | **tested:** `pinglymotion_aiassistant_tests` (`namesAreRememberedNotTheKey`). |
| An AI can save files only in the PinglyMotion work folder. It cannot replace another existing file or write outside that folder. | **tested:** `pinglymotion_control_tests` (`filesStayInWorkFolder`). |
| An AI can read LSL scripts and notecards in a folder it names (Script Lab Live, `lab_watch`), anywhere on this PC that your user account can read. It only reads them; it never writes there. | Reading: **tested**, `pinglymotion_control_tests` (`scriptLabLiveCommands`, on a temporary folder). That nothing is written: **not tested**, read in `src/scriptlab/labwatcher.*` and the command text in `src/controlapi.cpp`. |
| An AI can take pictures of the 3D view (`screenshot`). | **tested:** `pinglymotion_control_tests` (`screenshotIsPng` and the other `screenshot` tests). |
| PinglyMotion sends nothing to any AI company. The AI app itself may send what it reads (poses, pictures, script text) to its company's servers; that company's privacy rules apply. | PinglyMotion's side: **not tested**, same reading of the code as in section 1. What the AI app does is outside PinglyMotion and cannot be tested here. |
| The bridge for AI apps (`tools/mcp/pinglymotion_mcp.js`) talks only to `127.0.0.1` or `localhost`, never through a proxy, and refuses any other address in the connection file. | **tested:** `tests/test_mcp_node.py`. |

## 3. The live link to Aurora

| Claim | Proof |
|---|---|
| The link connects only to this PC. | **not tested.** `connectToHost(QHostAddress::LocalHost, ...)` in `src/auroralink.cpp`. |
| Both sides prove they know the same key before a pose is sent. A wrong key or a missing proof is refused. | **tested:** `pinglymotion_auroralink_tests` (`wrongKeyIsRefused`, `helloWithoutMutualAuthIsRefused`, `standInWithoutKeyGetsNoPose`, `wrongProofIsRefused`). |
| It is off each time PinglyMotion starts, until you turn it on in Options. | **not tested.** `m_enabled(false)` in `src/auroralink.cpp`; the menu entry in `src/pinglymotionwindow.cpp` is not restored from the settings. |

## 4. Files on your PC

| Claim | Proof |
|---|---|
| The error log stays on your PC. It is at most 5 MB, the oldest lines go first, and your home folder is written as `<home>`. | **tested:** `pinglymotion_applog_tests` (`limitIsFiveMegabytes`, `fileStaysUnderFiveMegabytesAndDropsOldestLines`, `homeFolderIsReplaced`, `homeFolderIsReplacedInTheFile`). That it is never sent: **not tested**, same reading as section 1. |
| Help > Clear log empties the log. | **tested:** `pinglymotion_applog_tests` (`clearEmptiesTheFile`). |
| Settings are stored on your PC only. | **not tested.** `QSettings` in `src/settings.*`; no network code reads them. |
| Damaged or hostile animation, model and script files are fuzzed every night and on request: `.anim`, `.dingle`, BVH, glTF, props, joint maps, LSL, RLV and the control commands. | **tested:** the fuzz workflow (`.github/workflows/fuzz.yml`, targets in `fuzz/`), and the sanitizer build (`asan-ubsan` check) on every pull request. |

## 5. How the project checks itself

| Claim | Proof |
|---|---|
| CodeQL (security queries) analyses the C++, Python and workflow code on pull requests and every night. Its open alerts are listed in the nightly run. | `.github/workflows/codeql.yml`; required check `analyze`. |
| New dependencies are checked for known problems on every pull request. | Required check `dependency-review`. |
| Every pull request is checked for personal data (names, e-mail addresses). | Required check `privacy`, `tools/check_privacy.py`, tested by `tests/test_check_privacy.py`. |
| Security alerts are fixed as ordinary jobs, and only counted as fixed when CodeQL confirms it. No alert is dismissed by hand. | The rule: `CLAUDE.md`, "The job queue", "Security alerts". |
| Each release has `SHA256SUMS` and a signed build attestation. | **not tested here.** `.github/workflows/release.yml`; how to check a download: [SECURITY.md](../SECURITY.md). |

## 6. Honest limits

What this page cannot promise yet:

- **No test blocks the network.** That the program makes no other connection is read in the code, not tested.
- **Windows by hand is not tested.** The automatic tests run on Linux and Windows, but nobody has checked these claims by hand on a Windows PC for this page. The key file's owner-only rights are not tested on Windows.
- **The update check request itself is not tested** (address, header, weekly timing, 1 MB limit). Only the reading of GitHub's answer is.
- **The Aurora link's "this PC only" is not tested**, only read in the code.
- **The AI app's name in the status bar is not checked.** Any program on this PC that has the key can give any name.
- **What an AI app does with what it reads** is decided by that app and its company, not by PinglyMotion.
- **Screen readers are not tested.** For example, the Script Lab editor's suggestion list has not been tried with a screen reader.
- **macOS** has no build and is not tested.

## 7. Legal text (GDPR)

> **Not written yet. This place is kept for the legal privacy text (GDPR).**
> It must be written or approved by the project owner or a lawyer before it is published. No AI session writes or changes this text on its own. Until then, the website's [privacy.html](privacy.html) is the published policy.
