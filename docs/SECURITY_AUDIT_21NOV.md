# PinglyMotion - security audit, 21 Nov 2026

Revision: `main` @ 8ca4474 (clone was identical to GitHub main). Read-only; nothing pushed, no issues or comments.
Method: static reading of src/, tools/, fuzz/, .github/; GitHub REST for settings, issues (243) and issue comments (239). No build (static reading was enough).
Limits: Actions settings and code-scanning/Dependabot alerts were not readable (proxy/permission 403). Status below is against the public docs/SECURITY_AUDIT.md (29 findings), docs/CODE_AUDIT.md and docs/CODE_ANALYSIS.md.

## Severity table

| Severity | Count | IDs |
|---|---|---|
| High | 0 | - |
| Medium | 2 | N-01, N-02 |
| Low | 7 | N-03, L-01, L-02, L-03, L-04, L-05, L-06 |
| Info | 7 | I-01 .. I-07 |

Earlier findings (docs/SECURITY_AUDIT.md): spot-checked on current main, still fixed (parser limits, control-server header/body/connection limits, Host/Origin check, work-folder path check, HMAC hello, MCP proxy bypass, connection file 0600/atomic, CI pinning + persist-credentials false in the main workflows, Qt 6.12). AL-03 (issue 23, live link does not verify Aurora) is now FIXED: client requires `auth-mutual` and checks `server_proof` before sending any pose (src/auroralink.cpp:326-370); issue closed. One regression of SC-01/SC-03 in a newer workflow: L-01.

## Findings

### N-01 Medium - control commands touch Windows network paths before the folder check (new)
- Area: remote control (src/controlapi.cpp:135-161, used by open_file 711, save_file 728, export_anim 1023)
- What: `resolveInWorkFolder` builds `QFileInfo(QDir(folder), path)` and calls `canonicalFilePath()` / `exists()` on the given path BEFORE the "inside the work folder" prefix check. On Windows an absolute UNC path (`\\host\share\x.bvh`, `//host/...`) makes Windows contact that host over SMB.
- Impact: an AI client steered by prompt injection (web page, file text) can make PinglyMotion open an SMB connection to an attacker host, which can leak the user's NetNTLMv2 hash. Needs remote control on and Windows; outbound 445 is often blocked, so Medium not High.
- Fix: refuse syntactically first, without any filesystem call: absolute paths, drive letters, `\\` or `//` prefixes, `:` anywhere, `..` segments, device names (CON, NUL, AUX, COM1...). Only plain names (or simple relative names) reach the filesystem. Safe test: unit test that `open_file` with `//example.invalid/s/a.bvh` fails and no `QFileInfo` call is made (inject a path-checker seam), plus a Windows test that the answer comes instantly.
- Status: fixed (21 Nov 2026, pull request 254, issue 244 closed). Was: new (the PM-08 fix covers the folder, not the order of checks).

### N-02 Medium - combined test build can pull in fork pull requests (new)
- Area: process / supply chain (tools/update_combined.sh:15-25, CLAUDE.md "One combined build")
- What: the script fetches `refs/pull/<n>/head` for any number and pushes the merge to `test/combined` in the main repository. It does not check that the pull request comes from this repository or from the owner account.
- Impact: if a session lists a fork PR (by mistake or because a third party opened a look-alike PR), unreviewed outside code is (a) run by the owner from the Windows zip and (b) run by workflows on push in the base repository, with workflow files the PR itself may change (token rights up to the repository default).
- Fix: in the script, for each number read `gh api repos/.../pulls/<n>` and stop unless `head.repo.full_name` is the base repository and `user.login` is the owner account; also pin the fetched head SHA. Safe test: dry-run the script with a known fork PR number (or a mocked API answer) and expect refusal.
- Status: fixed (21 Nov 2026, pull request 257, issue 245 closed). Was: new.

### N-03 Low - Kimodo helper installs unhashed, partly unpinned packages (new)
- Area: tools/kimodo/kimodo_helper.py:44-47, 460-470; kimodo_setup.ps1
- What: uv itself is SHA-256 pinned (good) and Kimodo is pinned to a commit zip, but `torch==2.8.0` is pinned by version only, and every transitive dependency of Kimodo/torch is resolved live from PyPI at install time without hashes. Model weights are fetched by Kimodo from Hugging Face without a pinned revision.
- Impact: a compromised upstream package or model repo runs code on the user's PC (opt-in helper, user's own rights).
- Fix: ship a `uv` lock / requirements file with `--require-hashes` for the full tree; pin the HF model revision (commit hash). Safe test: install in CI with `--require-hashes` and confirm it fails when one hash is changed.
- Status: not fixed yet: the hash-locked install is an open pull request (260; issue 247 is open). Was: new.

### L-01 Low - reference-fetch workflow regressed on action pinning (regression of SC-01/SC-03)
- .github/workflows/reference-fetch.yml:31,38,44: `actions/checkout@v4`, `actions/upload-artifact@v4` by tag, checkout without `persist-credentials: false`; the `compute-hashes` job also exports `GITHUB_TOKEN` to a script. Permissions are `contents: read`, so impact is limited.
- Fix: pin by full SHA like the other workflows, add `persist-credentials: false`; pass the token only if the script truly needs it.
- Status: fixed (21 Nov 2026, pull request 258, issue 246 closed).

### L-02 Low - Blender workflow installs unpinned tools
- .github/workflows/blender.yml:26-34: `pip install uv` and `uv run --with bpy` without versions or hashes. PR-only, `contents: read`, no secrets.
- Fix: pin uv and bpy versions (ideally hashes).
- Status: fixed (21 Nov 2026, pull request 258, issue 246 closed).

### L-03 Low - pre-auth connection slots can be held by any local process
- src/controlserver.cpp:35-36, 239-256: 8 connection slots, each held up to 15 s without the key; a local program can keep all 8 busy so the AI client only gets 503.
- Fix: short pre-header timeout (e.g. 2 s until the header end), and/or per-connection timer reset only after authentication. Low because it needs local code execution.
- Status: fixed (21 Nov 2026, pull request 261, issue 249 closed). Was: residual of PM-10.

### L-04 Low - load_reference checks the hash, then opens the file again by path
- src/controlapi.cpp:1406-1418 (hash) then `m_host->openFile(file)`; the index (`downloaded_index.json`) sits in the same user-writable folder, so the hash only detects damage, not tampering.
- Fix: parse from the bytes that were hashed (pass the buffer to the loader). Document that the downloaded library is trusted like any user file.
- Status: fixed (21 Nov 2026, pull request 268, issue 248 closed).

### L-05 Low - save_file "never replace" check is check-then-act
- src/controlapi.cpp:731-735: existence check, then `QSaveFile` rename can replace a file created in between. export_anim does it right with `NewOnly` (1058).
- Fix: for a new name, create with `NewOnly` first (or re-check in the save routine).
- Status: fixed (21 Nov 2026, pull request 264, issue 248 closed).

### L-06 Low - homepage lists titles of any open pull request, including forks
- tools/site/build_status.py:89-95: "building" list is built from all open PR titles (escaped, so no XSS). Anyone can put text on the project homepage until the PR is closed.
- Fix: keep only PRs where `head.repo.full_name` is the base repo.
- Status: fixed (21 Nov 2026, pull request 263, part of issue 252).

### I-01 Info - Aurora link: HMAC answer sent before the server proves itself; server text shown unclipped
- src/auroralink.cpp:340-350 sends HMAC(key, challenge) to a not-yet-verified peer (a squatter on the port gets one HMAC per connect; key is per port file, so no practical use found). src/auroralink.cpp:378 shows `error` text from the peer without a length cap.
- Fix (optional): clip shown text to ~200 printable characters.
- Status: not done: pull request 263 left the Aurora link items for a job with an owner test (issue 252 is open).

### I-02 Info - fuzz coverage gaps
- Fuzzed: AnimFile parse (BVH/AVM), Dingle, .anim parse, glTF read+import, props, retarget mapping, ControlApi::execute.
- Not fuzzed: building the BVH tree/Animation from a parsed file (BVH::animRead path), `.anim` to animation conversion (src/animimportconvert.cpp), BVH-from-another-skeleton retarget (Retarget::mapSkeletons on foreign names), the HTTP framing in ControlServer (onReadyRead/completeRequestLength), ReleaseInfo JSON.
- Fix: add 2-3 targets (open-file end to end, retarget BVH, control-server raw bytes).
- Status: fixed (21 Nov 2026, pull request 262, issue 250 closed): three more fuzz targets.

### I-03 Info - update checker robustness
- src/updatechecker.cpp:158-170 does not check `reply->error()` or cap the reply size; src/versionnumber.cpp:31 regex uses unescaped `.`. Nothing is downloaded or executed; only the tag is read over HTTPS from api.github.com. No action needed beyond tidy-up.
- Status: fixed (21 Nov 2026, pull request 263, part of issue 252).

### I-04 Info - untrusted text reaches the AI as tool output
- File names in the work folder and texts from reference indexes go into tool answers (prompt-injection channel). Impact is bounded by the work-folder rule; N-01 is the one path that escaped it.

### I-05 Info - private Claude session links in public PR bodies
- 27 PR/issue bodies (numbers 1-4, 29, 31-45, 48-50, 55-56, 166, 173, 178) contain claude.ai session links. Not personal data and not accessible to others, but CLAUDE.md bans session lines only in commits. Optional: extend the rule/privacy check to PR bodies.

### I-06 Info - repository settings and docs
- Dependabot security updates are disabled (only the actions ecosystem is configured for version updates); secret scanning and push protection on; private vulnerability reporting on; rulesets active. Code signing of the Windows exe still not done (known SC-03).
- SECURITY.md is stale: says latest release 0.2.x; scope lists .bvh/.avm/.prp but not .dingle, .anim, .glb/.gltf, the .mcpb extension, the Kimodo helper or the live link auth.
- Status: partly fixed: SECURITY.md was refreshed (21 Nov 2026, pull request 263); Dependabot security updates were still off when pull request 263 was written (a repository setting for the project owner) and code signing is not done.

### I-07 Info - Kimodo Hugging Face token stored in plain text
- tools/kimodo/kimodo_helper.py:217-224: 0600 file in the helper folder (Windows relies on the profile ACL). Acceptable for a read token; mention in the helper README.

## Privacy scan (public repo, history, issues)
- Tracked files: no personal paths, names, emails or account names found (only placeholders and test strings).
- Commit authors/committers via API: only the project noreply identity and GitHub.
- Issues and comments (243 + 239): no personal names, towns, emails or home paths found.
- PNG/GLB metadata: nothing personal.
- Only item: I-05 (session links).

## Suggested fix jobs (priority order)
1. N-01: syntactic path refusal before any filesystem call in resolveInWorkFolder, with a test (technical, mechanically testable).
2. N-02: update_combined.sh refuses PRs not from this repository/owner; pin head SHA (technical).
3. L-01 + L-02: pin actions by SHA and add persist-credentials false in reference-fetch.yml; pin uv/bpy in blender.yml (technical, light).
4. N-03: hash-locked install for the Kimodo helper and pinned model revision (technical).
5. L-04 + L-05: load from hashed bytes; NewOnly on first save (technical).
6. L-03: short pre-header timeout in the control server (technical).
7. I-02: three more fuzz targets (technical).
8. L-06, I-01, I-03, I-06 docs: homepage PR filter, clip Aurora error text, updater tidy-up, refresh SECURITY.md (light).

## Issues

| Finding | Issue |
|---|---|
| N-01 | #244 |
| N-02 | #245 |
| L-01, L-02 | #246 |
| N-03 | #247 |
| L-04, L-05 | #248 |
| L-03 | #249 |
| I-02 | #250 |
| L-06, I-01, I-03, I-06 | #252 |
