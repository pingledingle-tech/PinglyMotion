# Text-to-motion AI and Second Life: research report (issue 18)

Status: **research, nothing built.** Written 20 November 2026 for [issue 18](https://github.com/pingledingle-tech/PinglyMotion/issues/18). Sources were read on that date; prices and licences change, so check them again before acting. **The licence notes are a reading of public texts, not legal advice.**

Builds on the study in [issue 129](https://github.com/pingledingle-tech/PinglyMotion/issues/129) (an assistant needs the animation as computed text; language models plan movement well in words but judge it badly from pictures) and on [AI_HELP_PLAN.md](../AI_HELP_PLAN.md). That study ended with "a licence-checked text-to-motion trial" as a later step; this report is that licence check plus the integration choice. The earlier private project note on AI animation help could not be read from this session, so this report relies on the issue 129 study, which summarises it.

## What is new since issue 18 and the issue 129 study

1. **NVIDIA Kimodo exports BVH and its main models are commercially usable.** The SOMA models write BVH with a `--bvh` flag ([output formats](https://research.nvidia.com/labs/sil/projects/kimodo/docs/user_guide/output_formats.html)) and are under the NVIDIA Open Model License ([Kimodo README](https://github.com/nv-tlabs/kimodo)), which says models are commercially usable and claims no ownership of outputs ([licence](https://www.nvidia.com/en-us/agreements/enterprise-software/nvidia-open-model-license)). It also takes **full-body keyframes as constraints** ([paper](https://arxiv.org/abs/2603.15546)), which is exactly what a PinglyMotion user makes. This is the only open model found that is both licence-plausible for sold animations and a direct BVH source.
2. **Almost every academic model is trained on data that forbids commercial use.** MDM, MoMask, MotionGPT, MotionLCM, T2M-GPT and Light-T2M are trained on HumanML3D, built from AMASS; AMASS forbids training networks for commercial use ([AMASS licence](https://amass.is.tue.mpg.de/license.html)). The code being MIT does not change what the weights were trained on.
3. **HY-Motion 1.0 cannot be a default in Europe.** Its community licence excludes the EU, the UK and South Korea, as a question on its own tracker shows; Tencent answered only with a legal contact ([HY-Motion issue 49](https://github.com/Tencent-Hunyuan/HY-Motion-1.0/issues/49)).
4. **No open model makes fingers.** HY-Motion is 22 joints without hands ([paper](https://arxiv.org/html/2512.23464v1)); Kimodo's training data is body-only ([paper](https://arxiv.org/abs/2603.15546)); SayMotion says it is full body only ([docs](https://deepmotion.com/doc/saymotion)).
5. **Hosted services moved to pay-per-second and explicit commercial terms** (Uthana, Rokoko Create), see "Hosted services".
6. **PinglyMotion already has the import half of the bridge**: BVH from any skeleton with retargeting and hand-made `.jointmap` corrections, glTF import, and `describe_pose` for assistants ([RETARGET_MAPPING.md](../RETARGET_MAPPING.md), [CONTROL.md](../CONTROL.md)). What is missing is small and specific (see "Gaps").

## 1. Models (2024 to 2026)

| Model | Year | Code licence | Weights trained on | Output | Notes for us |
|---|---|---|---|---|---|
| MDM (and DiP, a fast variant) | 2022, updates 2025 | MIT ([repo](https://github.com/GuyTevet/motion-diffusion-model)) | HumanML3D, KIT-ML | Joint positions (`.npy`), optional SMPL fit; no BVH | Max 9.8 s; about 0.4 s per sample with the 50-step model |
| MoMask | 2024 | MIT ([repo](https://github.com/EricGuo5513/momask-codes)) | HumanML3D, KIT-ML | **BVH** (plus a "naive foot IK" copy that it says sometimes fails) | 20 fps; strong benchmark baseline; easiest to try |
| MotionGPT (v1), MotionGPT3 | 2023, 2025 | MIT ([repo](https://github.com/OpenMotionLab/MotionGPT)) | HumanML3D | Joint positions, SMPL meshes; no BVH | Language-model style; text in and out |
| MotionLCM | 2024 | **Non-commercial** licence ([repo](https://github.com/Dai-Wenxun/MotionLCM)) | HumanML3D | Joint positions, SMPL meshes | Real-time, trajectory control; licence rules it out |
| HY-Motion 1.0 (Tencent) | Dec 2025 | Tencent community licence, excludes EU, UK, South Korea ([issue 49](https://github.com/Tencent-Hunyuan/HY-Motion-1.0/issues/49)) | Over 3,000 h, mostly motion estimated from web video, about 500 h mocap and assets ([paper](https://arxiv.org/html/2512.23464v1)) | SMPL-H rotations, 22 joints, no hands, 30 fps | 1.0B and 0.46B parameters; 26 GB and 24 GB VRAM minimum ([repo](https://github.com/Tencent-Hunyuan/HY-Motion-1.0)); best human ratings against MoMask and others in its own paper |
| Kimodo (NVIDIA) | Mar 2026, v1.1 Apr 2026 | Apache-2.0 code; SOMA and G1 models under NVIDIA Open Model License; SMPL-X model research-only ([README](https://github.com/nv-tlabs/kimodo)) | RP: Bones Rigplay, about 700 h proprietary mocap; SEED: public BONES-SEED, about 288 h | **BVH** (SOMA, 77 joints), NPZ, AMASS NPZ (SMPL-X) | Keyframe, end-effector, path and contact constraints; 30 fps; trained clips up to 10 s; 2 to 5 s per motion on an RTX 3090 ([paper](https://arxiv.org/abs/2603.15546)); about 17 GB VRAM, under 3 GB with the text encoder on the CPU |
| ARDY (NVIDIA) | 2026 (SIGGRAPH) | Not stated on the project page ([page](https://research.nvidia.com/labs/sil/projects/ardy/)) | RP data per its model names | Streaming motion | Interactive and real-time; Kimodo is NVIDIA's offline authoring model, so Kimodo fits an editor better |
| SnapMoGen / MoMask++ (Snap) | 2025 | Release and licence not stated ([paper](https://arxiv.org/html/2507.09122v1)) | Own mocap, 43.7 h, 30 fps | 24 joints | Long expressive prompts; watch for a release |
| OpenT2M / MonoFrill | 2026 (CVPR) | No licence named ([paper](https://arxiv.org/pdf/2603.18623)) | About 2,800 h, mostly from web video | Own body-part format | Related work also lists Being-M0.5, MotionMillion, Motion Anything, ScaMo |

Trend: data is scaling by estimating motion from web video (HY-Motion, OpenT2M), and control is moving from text only to text plus keyframes and paths (Kimodo, ARDY). For an editor the second trend matters more: it turns the model into an **inbetweener for the user's own key poses**, not a black box.

## 2. Datasets and licences (not legal advice)

Selling animations in Second Life is commercial use. So what counts is what the weights were trained on, not the code licence.

| Data | Terms found | Effect |
|---|---|---|
| AMASS | Non-commercial research, education or art only; training networks for commercial use is prohibited; no redistribution ([licence](https://amass.is.tue.mpg.de/license.html)) | Taints every model trained on it for sold animations |
| HumanML3D | Repo is MIT, but the motion comes from AMASS and HumanAct12 and cannot be redistributed because of AMASS ([repo](https://github.com/EricGuo5513/HumanML3D)) | Same as AMASS |
| KIT-ML | No licence found on its page ([KIT page](https://h2t.iar.kit.edu/1445.php)) | Unclear; treat as research-only until confirmed |
| Motion-X | Annotations CC BY-NC-SA 4.0; sub-sets under their own licences ([licence](https://motion-x-dataset.github.io/static/license/Motion-X%20License.pdf)) | Non-commercial |
| SMPL body model | SMPL-Body subset CC BY 4.0, the full model has its own licence and patents ([SMPL licence page](https://smpl.is.tue.mpg.de/license.html)) | Matters only if we ship SMPL files; we do not need to: BVH joint names and rotations are enough |
| BONES-SEED | Own "bones-seed-license", must be accepted before download ([dataset page](https://huggingface.co/datasets/bones-studio/seed)) | Not read in full; Kimodo calls its training data commercially friendly ([README](https://github.com/nv-tlabs/kimodo)) |
| CMU mocap, Mesh2Motion | Free for commercial use (CMU, quoted) and CC0 ([OPEN_SOURCE_CHECK.md](../OPEN_SOURCE_CHECK.md)) | Already bundled as test motions; too small to train a model |

Conclusion: an AMASS-trained model (MoMask and friends) is fine for a user's **private experiments**, but PinglyMotion should neither bundle its weights nor present its output as sellable. Retraining on clean data (CMU, Mesh2Motion CC0, BONES-SEED) is not realistic for this project, as issue 18 already said.

## 3. Hosted services

| Service | Text-to-motion | Price found | Export | Output terms found |
|---|---|---|---|---|
| DeepMotion SayMotion | Yes, plus inpainting and blending ([product](https://www.deepmotion.com/saymotion)) | Pricing page could not be read | FBX, GLB, **BVH**, MP4; body only, no hands or face ([docs](https://deepmotion.com/doc/saymotion)) | Not found; check before selling |
| Uthana | Yes, versions 1.0 to 3.0, plus stitching and looping | Pay per second: $0.02 to $0.10 per second of motion; looping $0.01 per second ([pricing](https://uthana.com/pricing)) | Not stated on the pricing page | Paid use includes commercial use of outputs, under its terms |
| Rokoko Create | Yes, own model, browser app ([page](https://rokoko.com/products/studio/rokoko-create)) | Free to start; paid plan after 5 clips added to Studio | "Custom formats"; Rokoko Studio exports FBX and BVH (third-party summary, [Cinevva](https://app.cinevva.com/guides/paid-rigging-animation-tools-2026)) | Commercial use allowed, under its terms |
| Krikey | Yes ([pricing](https://www.krikey.ai/pricing)) | Pro $15 per month | FBX on paid plans; BVH not listed | Not stated |
| Meshy, Tripo | Mostly animation presets on a rigged model; text-to-motion via API (third-party summary, [Cinevva](https://app.cinevva.com/guides/paid-rigging-animation-tools-2026)) | About $20 per month for commercial plans | FBX | Free tiers non-commercial or CC BY |
| Adobe, NVIDIA hosted | No hosted text-to-motion product found | | | Mixamo is presets, and its terms block passing raw files on ([DECISIONS.md](../DECISIONS.md)) |

All of these work today with PinglyMotion as option (a): export BVH or FBX, convert FBX to BVH or glTF in Blender if needed ([BLENDER.md](../BLENDER.md)), import with retargeting.

**What Second Life creators do today:** no public discussion of text-to-motion use for Second Life was found; the forum threads found are about BVH and `.anim` conversion, and could not be opened from this session ([example](https://community.secondlife.com/forums/topic/519910-converting-bvh-to-anim/)). The usual route stays Blender with paid add-ons, or motion capture, then upload. A bridge would be new, which matches issue 18's point that nobody has built it.

## 4. Quality against what Second Life needs

The Second Life rules are in [ASSISTANT_GUIDE.md](../ASSISTANT_GUIDE.md). Against them:

| Need | What models give | Work for PinglyMotion |
|---|---|---|
| Skeleton | SMPL 22 joints (HumanML3D, HY-Motion) or SOMA 77 (Kimodo) | Map: pelvis to `hip`, spine1 to `abdomen`, spine3 to `chest` (spine2 folded in or to Bento `mSpine`), collar, shoulder, elbow, wrist to `lCollar`, `lShldr`, `lForeArm`, `lHand`, hip, knee, ankle to `lThigh`, `lShin`, `lFoot`, foot to `mToeLeft`, neck and head. The retargeting core does this by name and tree today; a ready mapping for these two skeletons removes the guesswork |
| Rotations, not positions | MDM, MotionGPT and MotionLCM give joint **positions**; turning them into rotations loses forearm and head twist | Prefer sources with rotations: MoMask BVH, Kimodo BVH, HY-Motion SMPL-H, SayMotion BVH |
| Hands and fingers | None | Fingers stay as posed in PinglyMotion, or the `.anim` hand pose. Fine for most sits, stands and walks; gestures need hand keys by hand or by an assistant |
| Frame rate | 20 fps (HumanML3D models, MoMask) or 30 fps (HY-Motion, Kimodo) | Resample to 30 on import |
| Length, 60 s limit | Trained clips 2 to 10 s; MDM caps at 9.8 s | Long dances need several clips joined; Uthana and SayMotion offer stitching |
| Loops | No model makes a seamless loop by itself | A loop-seam blend at import or in the editor; `check_animation` (planned) reports the seam |
| Walking in place | Models move the root forward | Second Life moves the avatar itself, so an AO walk must stay in place: remove forward hip travel on import |
| Frame 1 reference | Models start in motion | Insert the start pose at frame 1 and begin the motion at frame 2 |
| Foot sliding, floor | Common; MoMask's foot IK "sometimes fails"; Kimodo has a foot-skate metric and contact constraints | `check_animation` reports it; a later foot-lock tool |
| Priority, ease, hand pose | Not part of the model | Set in the `.anim` export, as today |

Quality ranking from the papers (not tested here): HY-Motion rated above MoMask by humans in its own study; Kimodo adds control. Expect usable everyday motion (walk, wave, sit down, idle) and weaker results for SL staples that are rare in mocap data (lying poses, couples, sensual dances, furniture sits).

## 5. Compute

| Route | Need | Comment |
|---|---|---|
| MoMask locally | NVIDIA GPU, old PyTorch, Linux tested ([repo](https://github.com/EricGuo5513/momask-codes)) | Small model; its web demo runs on CPU |
| Kimodo locally | About 17 GB VRAM, or under 3 GB with the text encoder on the CPU; developed on Linux, Windows "should work", Docker suggested ([README](https://github.com/nv-tlabs/kimodo)) | A mid-range NVIDIA card can run it in the low-VRAM mode; setup is the hurdle for beginners |
| HY-Motion locally | 24 to 26 GB VRAM ([repo](https://github.com/Tencent-Hunyuan/HY-Motion-1.0)) | High-end cards only, and the territory problem |
| Rented cloud GPU | Hourly GPU rental, a Linux image | Fine for a trial; not something a beginner sets up |
| Hosted service | A browser and an account | Easiest; cost per second or per month; their terms decide resale |

Second Life users run Windows and often laptops without a large NVIDIA card. Any local model must stay optional and outside the zip.

## 6. Integration options

**(a) The user imports BVH or glTF from an external tool (exists).** Works with every service above and with MoMask or Kimodo run by a power user. Licence responsibility stays with the user and the tool's terms. Gaps: no ready mapping for SMPL-22 and SOMA-77; no "in place", resample, frame-1 insert or loop-seam steps for generated motion; no control command to import, so an assistant cannot do it.

**(b) An assistant composes the motion through the control commands (in progress).** Claude sets key poses with `set_pose`, reads them back with `describe_pose`, and soon `describe_motion` and `check_animation` (issue 129). No licence questions and no GPU. Good for poses, sits, gestures and simple loops; weak for natural full-body motion such as walks and dances, which is the reason issue 18 exists. The two routes meet: the assistant writes the precise motion prompt for the external tool, the user imports the result, and the assistant then checks and fixes it with the commands.

**(c) A bundled local model.** Rejected for the zip: gigabytes of weights and PyTorch with CUDA, licence problems for every AMASS-trained model, and only Kimodo as a licence-plausible choice, which needs a large GPU and a Linux-first setup. If done at all, as an **optional external helper** (a documented script in `tools/` that writes BVH into the work folder), never inside the program.

**(d) An optional cloud service.** Possible later through a provider's API (Uthana has one), but it brings API keys, payments, personal data and terms of service into the program, and ties PinglyMotion to one vendor, against [PRODUCT.md](../PRODUCT.md) ("avoid tying the editor to a single assistant"). Not now.

## 7. Recommendation

1. **Make option (a) excellent first.** Ready mappings for the two common AI skeletons, and an import clean-up for generated motion (resample to 30 fps, start pose at frame 1, in place, loop seam). This helps every tool at once, costs no licence risk for the project, and reuses the retargeting core.
2. **Keep building option (b)** as planned in issue 129, and add an import command so an assistant can bring in a generated BVH and check it with `check_animation`.
3. **Trial Kimodo-SOMA as an external, optional helper** after 1 and 2, on a machine with an NVIDIA GPU: the only open model with BVH output, keyframe constraints and a licence that appears to allow sold results. Record its licence notice and BONES-SEED terms before any documentation recommends it.
4. **Do not bundle any model, and do not recommend AMASS-trained models or HY-Motion for animations that will be sold.** Say so in the user docs, as was done for Mixamo.

## 8. Risks

- **Licences (biggest):** most open models are research-only through their training data; Kimodo's own terms look permissive, but the BONES-SEED licence and NVIDIA's guardrail clause were not read in full. Not legal advice; a user selling animations should read the terms of the tool they use.
- **Hardware and setup:** a local model needs an NVIDIA GPU and a Python setup that beginners will not manage; this stays a power-user path.
- **Quality gaps:** no fingers, short clips, no loops, foot sliding, few SL-typical motions in training data.
- **Vendor drift:** services change prices and terms often (Uthana stopped new subscriptions in August 2026, per its pricing page); the docs must date every claim.
- **Aurora's rule:** no AI in the viewer. All of this stays in PinglyMotion and outside tools.

## 9. Next jobs (each small, each needs the project owner's go)

### Job 1: ready joint mappings for SMPL-22 and SOMA-77 (technical)

- **What:** the automatic match (or two bundled `.jointmap` files) recognises the HumanML3D/SMPL 22-joint skeleton (as MoMask writes it) and Kimodo's SOMA 77-joint skeleton.
- **Acceptance tests:**
  - Two small BVH fixtures **generated by a script in the repo** (our own data, only the joint names and tree copied as facts; no third-party motion) open through the retarget import with every body joint matched to the expected Second Life joint, no required joint unmatched, and finger joints left out without error.
  - A known +45 degree bend on the source left elbow at frame 10 arrives as the same bend on `lForeArm` within 1 degree.
  - Unknown extra joints in the SOMA fixture are left out and listed, not refused.
- **Files:** `src/retarget.*` or `bin/data/` mapping files, `tests/retarget*/`, `tools/` fixture generator, `docs/RETARGET_MAPPING.md`.

### Job 2: import clean-up for generated motion (technical core; menu later as a visible job)

- **What:** functions, then an import option: resample to 30 fps, put the start pose at frame 1, remove forward hip travel ("in place"), and blend the loop seam.
- **Acceptance tests:**
  - A 20 fps, 4 s fixture becomes 30 fps with the same length within one frame, and a joint's angle at 2.0 s matches within 0.5 degrees.
  - After "in place" the hip's horizontal position at the last frame equals frame 1 within 0.01 units; height is unchanged.
  - After the loop blend the last frame equals the first within 0.5 degrees per joint, and `check_animation` (when merged) reports no seam jump.
  - Frame 1 holds the start pose and frame 2 is the first moved frame.

### Job 3: Kimodo trial as an external helper (research spike, needs an NVIDIA GPU)

- **What:** run Kimodo-SOMA from its own repo in low-VRAM mode on a rented or local GPU, generate three clips (wave, walk in place, sit down from standing) and one keyframe-constrained clip made from a PinglyMotion `.dingle` key set, import them, export `.anim`, and write a short dated guide with the licence notice.
- **Acceptance tests:** the four BVH files import with Job 1's mapping and no unmatched body joint; `check_animation` reports no Second Life error after Job 2's clean-up; the `.anim` export passes the existing export tests; the guide names the exact model version and quotes its licence name; no weights or generated motion are committed unless their licence allows it.
- **Blocker:** a background session has no GPU; this job needs a machine with one, or it stops and reports.
