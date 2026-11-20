# Reference library plan (issue 19)

Status: job 1 (index and 55 curated clips) built, format in `docs/REFERENCE_LIBRARY_SCHEMA.md`; Quaternius not yet bundled; job 2 built, job 3 first part (the control commands `find_reference` and `load_reference`) built; the browser panel waits for the owner. Written 20 November 2026. Waiting for the project owner's go. Licence readings below come from the sources' own pages, fetched on this date. **This is not legal advice.** Where a point is uncertain it is marked **UNCERTAIN**.

## Goal

A searchable set of starting poses and motions for users and AI assistants (issue 19), fed later by community poses under CC0 (issue 20). Large data stays out of the repository; what is bundled is small, clearly licensed and credited.

## What is already in the repository

- Eight CMU clips (`tests/motions/cmu/`, `tests/motions/cmu_02_01_walk.bvh`), converted with `tools/cmu_to_sl.py`, acknowledgment in `tests/motions/README.md`.
- Eight CC0 Mesh2Motion clips (`tests/data/mesh2motion/`, two of them also as BVH), credited in Help > About.
- Import paths exist for BVH from another skeleton (with hand-made joint mapping) and glTF animation.

## Sources

| Source | Offers | Licence (as read) | Bundle in the open-source app? | Redistribute? | Sell derived SL animations? | Attribution |
|---|---|---|---|---|---|---|
| CMU Graphics Lab Motion Capture Database | About 2 500 trials of many subjects (commonly cited; not counted by us). Original ASF/AMC. BVH only through third-party conversions (cgspeed, Poser-friendly copy). | The CMU FAQ says the data "may be copied, modified, or redistributed without permission". The site gives no formal licence text and no mention of commercial use. The cgspeed README (already quoted in `docs/OPEN_SOURCE_CHECK.md`) says free for commercial use with no restrictions. | Yes, a small selection. Already done for 8 clips. | Yes, per the FAQ sentence | Probably yes, because CMU states no restriction. **UNCERTAIN:** no SPDX licence, no signed grant; the commercial wording comes from the third-party README | CMU asks for its acknowledgment text (NSF EIA-0196217). Keep it. Quality is not guaranteed by CMU. |
| Mesh2Motion animations (`Mesh2Motion/mesh2motion-app`) | 234 humanoid `.glb` clips on one skeleton, plus a rest pose. Quality is varied; the author says he is not an animator. | Models, rigs and animations CC0. Code MIT. | Yes, any number, size permitting | Yes | Yes | None required. Credit given anyway (owner's decision, `docs/DECISIONS.md`). **UNCERTAIN:** CC0 is the project's claim; the clips were made in Blender by the author, not checked clip by clip. |
| Quaternius Universal Animation Library (standard, pro) | 120+ animations on one humanoid rig, glTF/FBX | CC0, "free to use in personal, educational and commercial projects" (page). The paid "Source" tier is a separate purchase. | Yes for the free tiers, after a re-check of the download page and file headers | Yes (CC0) | Yes | None required. Credit as courtesy. |
| Linden Lab built-in SL animations | The viewer's own animations (walk, stand, sit, hand poses). The SL wiki links only a "default pose" BVH inside `avatar_mesh.zip`; no set of Linden animation BVH files is offered there. | No licence for the BVH or ZIP is stated. The wiki text is CC BY-SA 3.0; that covers the wiki, not the ZIP. The viewer code is LGPL-2.1; the page does not say how the character assets are licensed. | **Do not bundle.** **UNCERTAIN** | Do not redistribute | Do not rely on it | The issue text ("licence to check") is answered: not confirmed. Link to the wiki only. Our own reference frame comes from `bin/data/Relaxed.bvh`, already in the repo. |
| Mixamo (Adobe) | Thousands of clips, auto-rigging | Adobe forum FAQ (2022 post about the free preview, so check current Adobe terms): free commercial use inside your own project, but redistributing raw character or animation files is not allowed. Training machine-learning models on it is excluded. | **No.** | **No** | Using a Mixamo clip as the base of an SL animation sold in-world is **UNCERTAIN** (derived work, not raw redistribution). Not our call. | Not applicable |
| Ruth2 / Roth2 repositories | Body meshes; no animation set found | AGPL for the repository unless noted | Already used as bodies; nothing to add for the library | Per AGPL | n/a | Already credited |
| Bandai Namco Research motion dataset | About 420 000 frames, stylised BVH | CC BY-NC 4.0 (README) | **No** (non-commercial; our users may sell animations) | Only non-commercially | No | n/a |
| Ubisoft LAFAN1 | BVH, 137 MB zip | CC BY-NC-ND 4.0 | **No** (non-commercial, no derivatives; retargeting is a derivative) | No | No | n/a |
| AMASS and similar academic sets | Large SMPL data | Non-commercial; may not be made available to third parties without permission | **No** | **No** | No | n/a |
| Community poses (issue 20) | Poses written by SL animators | CC0, contributor confirms they are their own or openly licensed | Yes, after owner review | Yes | Yes | Contributor credit optional |
| Marketplace poses, closed bodies | Locked inside SL | No open licence | No | No | No | Only with written permission from the creator |

Not checked: KIT Whole-Body Motion Database, HDM05, the 100STYLE set, Truebones free packs. Check each before use.

## Recommendation

**Bundle** (small, in the Windows zip under `examples/`, credited in Help > About and the README):
- the existing CMU and Mesh2Motion clips,
- a curated set of about 30 to 60 more clips, chosen for what SL animators need (sit, stand, dance, walk, gestures), from CMU, Mesh2Motion and Quaternius, each converted to the SL skeleton and tagged,
- later, community CC0 poses after owner review.

**Link or download on demand** (not stored in the repository):
- the full CMU set through a download script that fetches the cgspeed BVH files and converts them with `tools/cmu_to_sl.py`; the user's machine downloads it, we do not host it,
- the rest of Mesh2Motion and Quaternius clips, fetched by the same script by commit hash, with a check of the file's SHA-256.

**Do not use:** Mixamo, Bandai Namco, LAFAN1, AMASS, Linden built-in animations (until Linden states terms), Marketplace content.

## Index

One `index.json` next to the data: id, name, short description, tags (for example "sitting, calm"), source, licence, credit text, length, frame rate, skeleton, file path or download address, SHA-256. The licence and credit fields are required, so the library can never show a clip without its terms. An assistant can write the first tags once; the owner reviews them (tags are text assistants follow, so a contributed tag could carry hidden instructions).

## How the user meets it (short, the layout is the owner's call)

A search box with tag filters and a list with name, length and source. Pick one to open it as the current motion, or to insert it as a pose. The credit line is always visible. Assistants get control commands: `find_reference "sitting"` returns a few hits, `load_reference <id>` opens one. A clip that is not downloaded yet says so and offers the download.

## Next jobs

All are small, one at a time. Job 1 is technical. Job 2 is technical (control commands, nothing on screen). Job 3 changes the screen and needs the owner.

1. **Index and curated clips (technical).** Add `index.json` with a schema check, plus 30 to 60 curated clips converted to the SL skeleton.
   - Acceptance: a test loads `index.json`, fails if any entry lacks id, tags, licence or credit text, and fails if a listed file is missing or its SHA-256 differs. A test opens every bundled clip and checks 19 joints and frame 0 equal to the SL rest pose. A test checks that every source used appears in the About credits text. A privacy check passes.
2. **Download script for the full sets (technical). Built: `tools/fetch_reference.py`, see `docs/DEV.md`; Quaternius and the conversion step are left out.** `tools/fetch_reference.py` downloads the CMU BVH conversion and chosen Mesh2Motion and Quaternius clips by pinned address and hash, converts them, and writes entries into a local index.
   - Acceptance: with a local mirror served in the test, the script produces the expected files and index entries; a wrong hash stops it with a clear message and writes nothing; running it twice changes nothing; it never writes outside the target folder.
3. **Find and load commands (technical) and then the browser (visible, owner's test).** First `find_reference` and `load_reference` in `src/controlapi.*` and `tools/mcp/commands.json`, using the index. Then a small browser panel, only after the owner has said how it should look.
   - Acceptance for the commands: searching "sit" returns the sitting clips from the index in a fixed order and at most N hits; an unknown id gives an error and changes nothing; the fuzz run is green. The panel's acceptance is set with the owner before it is built.

## Open points for the owner

- Whether to contact CMU for a written statement of terms, which would turn the CMU **UNCERTAIN** into a clear grant.
- Whether the library may include clips whose use in sold SL animations is unclear (Mixamo is excluded in this plan).
