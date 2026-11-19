# Easing choices for beginners: names and descriptions

Plan only. Nothing in this document is in the program yet. It fixes the **names, the small pictures and the descriptions** of the six easing choices proposed in [#53](https://github.com/pingledingle-tech/PinglyMotion/issues/53), so the later jobs (the maths, the file format, the tile row on the timeline) all use the same words. The wording of the hint under each tile lives in [TEACHING.md](TEACHING.md) (IDs `EASE-01` to `EASE-06`); this page does not repeat it, so there is one place to change.

The six choices, their order and "Slow both ends" as the default are proposals in #53. They are not decisions yet; if the project owner changes one, change it here and in `TEACHING.md`.

## Words

- **Easing**: how a movement speeds up or slows down between two keys (saved poses).
- **Choice**: one of the six named options. The user never sees the word "curve" or "ease in/out" (see the plain-word table in [TEACHING.md](TEACHING.md)).
- **Baking**: adding extra in-between keys so that the straight lines Second Life plays between keys follow the curve. Export does this automatically ([ANIM_FORMAT.md](ANIM_FORMAT.md)).

## The six choices

The order is the order of the tiles, left to right. A choice belongs to one pose change (from one key to the next) and is stored on the target key.

| # | Name on the tile | Tile picture (our own, simple line drawing) | What happens | Built from | Hint |
|---|---|---|---|---|---|
| 1 | Even | A straight diagonal line | One steady speed from the old pose to the new one | Neither switch | EASE-01 |
| 2 | Slow start | A line that is flat at the left and then climbs | Leaves the old pose gently, then speeds up | The start key's `easeOut` | EASE-02 |
| 3 | Slow finish | A line that climbs fast and flattens at the right | Moves at speed, then settles into the new pose | The target key's `easeIn` | EASE-03 |
| 4 | Slow both ends | A smooth S shape | Starts and stops softly | Both switches | EASE-04 |
| 5 | Overshoot, then settle | A line that rises above the end level and comes back down to it | Goes a little past the new pose, then returns | New stored value (`curve`) | EASE-05 |
| 6 | Hold, then snap | A flat line that jumps up at the right | Stays in the old pose, then jumps to the new one | New stored value (`curve`) | EASE-06 |

"Built from" is the expectation in #53. The first job of #53 must confirm by reading the code what the two switches do today, and may change this column.

## Rules for the names and pictures

- Names are short (at most three words), plain English, no animation jargon, and each starts with what the user sees, not how it is made.
- The default for a new key is "Slow both ends" (proposal 1 in #53). The tile row shows which choice the selected pose change uses.
- Each tile has its name under a small picture. The picture shows the movement from the left (old pose) to the right (new pose) as a line; it never shows numbers or handles.
- The pictures are drawn for this program. We do not copy icons, preset pictures or help text from other programs (copyright note in #53, and `DECISIONS.md` for EffectCraft and Aurora: ideas only).
- Overshoot is clamped to the joint limits (proposal 4 in #53), so a tile never lets a joint go past what Second Life allows. A bounce choice is left out for now (proposal 6).
- Per-joint choices and free curve handles are not part of the first version; they would sit under "Advanced" later.

## Where each part is built

| What | Job in #53 |
|---|---|
| Maths for the six choices, no screen change | Step 1 |
| Four old-style choices use today's switches | Step 2 |
| `curve` stored in `.dingle` ([DINGLE_FORMAT.md](DINGLE_FORMAT.md)) | Step 3 |
| Export test for a curved ease | Step 4 |
| Tile row and hint on the timeline (visible, the project owner tests) | Step 5 |
| The example wave with a note on each change ([TEACHING.md](TEACHING.md), `WAVE-02` to `WAVE-05`) | Step 6 |
