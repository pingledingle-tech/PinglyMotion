# Open-source check: tools we could learn from or use

Status: **research notes, nothing built.** Written 19 November 2026. Shallow clones of each project were read; nothing was built or run. "Licence" means what the project's own files say. This extends `docs/REUSE_REFERENCES.md`.

## Summary

| Project | Licence (from its files) | What it is | Use for us |
|---|---|---|---|
| Mesh2Motion (`Mesh2Motion/mesh2motion-app`) | Code MIT (`LICENSE-MIT.MD`). Models, rigs and animations CC0 (`LICENSE-CC0.MD`) | Web app (TypeScript, three.js): import a model, map bones, retarget, export animations | **Best find.** Retargeting logic can be ported to C++ with the MIT notice kept. Its 234 human animations are CC0 |
| CMU Graphics Lab Motion Capture Database (`mocap.cs.cmu.edu`), BVH conversion by cgspeed.com, copy in `una-dinosauria/cmu-mocap` | No licence file in the copy. The conversion's `READMEFIRST.txt` quotes CMU: free for research and commercial projects worldwide, no restrictions, and the conversion adds none. CMU asks for an acknowledgment text | Motion capture clips in BVH (CMU skeleton) | Eight clips are bundled as test motions (`tests/motions/cmu/`, issues #27 and #3) with the acknowledgment in the README. Basis is the quoted CMU terms, not an SPDX licence |
| sketchpunklabs/threejs_proto | `package.json` says ISC; no LICENSE file | Source of Mesh2Motion's swing-twist retargeter | Credit it. Write our own version from the idea, not line by line |
| anim-to-bvh | MIT (`package.json`) | Parses `.anim`, writes BVH, table of SL joint names and BVH aliases | Reference for #10 and #22. The SL viewer source stays the main reference |
| tanimbomb | No licence file found | Python tools for `.anim`: mirror, trim, priority, joint removal | Ideas and test ideas only until a licence is confirmed |
| Mocapy | MIT | Video or webcam to BVH with MediaPipe | External tool. Its BVH output goes in through our BVH import. No code in PinglyMotion |
| FreeMoCap | **AGPL-3.0** | Multi-webcam motion capture | External tool only. Never copy code |
| Pose2Sim | BSD-3-Clause | Markerless kinematics, OpenSim output | Not relevant (not BVH) |
| MocapNET | **Proprietary licence** that forbids integrating or distributing the library | Real-time 3D pose to BVH | Do not use |

## Mesh2Motion retargeting (`src/retarget`, about 3 200 lines with tests)

- `BoneNameTokenizer`: splits a bone name into tokens, finds the side (L/R/left/right) only when it stands alone, strips prefixes such as `mixamorig`, `Bip01`, `DEF-`, `ORG-`, finds end bones.
- `BoneSlotVocabulary`: ordered rules from token to canonical slot (pelvis, spine, neck, head, clavicle, upper arm, lower arm, hand, thigh, calf, foot, ball, finger slots, tail, wing). Rejects twist, roll, IK, pole and control bones on purpose.
- `BoneChainResolver`: resolves ambiguous names from the hierarchy ("shoulder" is the clavicle in Mixamo and the upper arm in DAZ/Poser).
- `CanonicalSlotMapper`: pairs chains of different length (a 4-bone spine against 3; fingers with 3 against 4 segments). Spine, tail and wing spread evenly; limbs and fingers align from the root.
- `Retargeter` and `RetargetUtils`: swing-twist retarget baked per frame; captures rest transforms (also from bind matrices); optional root correction for tilted rigs.
- Stated limit: required chains must be found or it refuses and names the missing bones. It knows nothing about Second Life: no `mPelvis`, no Bento spine, no collision volumes, no face bones.

### What this means for #16 and #12 (estimates, not built)

- Port tokenizer, vocabulary, chain resolver and slot mapper (about 800 lines) to C++, add SL and Bento slots (aliases from `avatar_skeleton.xml` and anim-to-bvh's table), keep the MIT notice in the source and in the third-party list.
- Port the swing-twist retarget with Qt maths. Rest-pose handling is where the bugs will be (A-pose against T-pose, different bone axes).
- Test with the Mixamo X Bot and Y Bot and a Rigify rig, and compare with the Blender route.
- Estimate: 2 to 4 working nights with Claude Code. This is a guess.

## CC0 test motions (#27)

`static/assets/human/animations/` in Mesh2Motion holds 234 `.glb` files for one humanoid skeleton, plus `rest-pose.glb`. The licence file says all models, rigs and animations are CC0. **Check the file headers and the licence once more before bundling any of them.** Useful picks: walk, walk-relaxed, walk-female, jog, sprint, idle-a, idle-talking, sitting-idle, kneel, dance-simple, dance-charleston, yes, greeting, clapping, victory, pray, meditate; hard cases: backflip, crawl, pushup, roll, swim-fwd, climb-ladder.

Four failure files in `static/test-files/import custom animations/` (wrong bone names, wrong bone count, no animation data) can test error messages in the glTF import (#12).

Using them needs glTF animation import (#12), or a one-off conversion to BVH.

## Not checked

Black Dragon and Alchemy source, Anim Hacker source, the licences of ReNim and Diffeomorphic `retarget_bvh`, the MediaPipe licence (believed Apache-2.0, not verified), Kinetiq Engine, PoseForge (no source found).
