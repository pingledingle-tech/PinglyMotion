ID: WP-02
Title: AI Control API and Script Lab Tooling
Status: waiting
Owner-approved: no
Priority: 2
Reason: Planned and ready; waiting for the project owner's go-ahead.

## Tasks

- [ ] Expose world positions for all joints on all frames in the control API (former job 29)
- [ ] Add `compare` control command to diff two animation versions across all frames and joints (former job 30)
- [ ] Add `save_script` control command to write `.lsl` files inside the watched Script Lab folder (former job 34; requires green fuzz)
- [ ] Support chains/leashes attached to an external object outside the watched object (former job 35)

### Part B — Better hands and fingers (moved here from WP-06, issue #532)

Owner decision 28 Nov 2026 14:55: the whole finger plan belongs in WP-02, including per-joint priority in export. Assessment first: the repo already has 30 finger joints, a Hands tab with 9 presets, and finger tips in `describe_pose`; the gaps are in the AI control API and checks. Rules: the program calculates, the AI chooses. Mesh hands only follow Bento fingers; `hand_pose` does not work on them.

- [ ] B1 Calibrate finger joints in the hand's own axes (bend, spread, twist per joint, both hands; thumb gets its own words); record limits and the mid/outer joint coupling
- [ ] B2 Add `set_hand` control command (hand, preset, bend per finger 0–100, spread, thumb); program computes the 30 joints with correct signs; reuses `src/handpose.*`
- [ ] B3 Extend `describe_pose` with bend % per finger, spread, nearest preset and similarity, contacts (thumb tip to finger tip, finger to palm) and crossing fingers
- [ ] B4 Add finger guide to `docs/ASSISTANT_GUIDE.md`: hand words, calibration table, human limits, sign trap, 15–20 recipes with `describe_pose` reference values
- [ ] B5 Add fingers to `check_animation` and `describe_motion` (warnings only: bent backwards, thumb out of range, crossing, fingers missing from the animation); contact checks use the joint world positions from the first task above
- [ ] B6 Add automated finger test set (15–20 fixed sentences with numeric expectations and tolerances) on `tests/assistant/`, measured before and after
- [ ] B7 Add per-joint priority to `export_anim` and the export dialog (GUI, with automated tests) and a note that hand pose only works on the old body, not on mesh hands
- [ ] B8 Optional for the owner: one hand pose tested in Second Life, collected with the next Windows test

Order: B1, B2, B3, B4, B5, B6 are technical and may merge when checks are green; B7 is GUI. Model: strong.
