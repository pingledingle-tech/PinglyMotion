# Research: Second Life animations from video (issue 17)

Status: **research report, nothing built.** Written 20 November 2026 for
[issue 17](https://github.com/pingledingle-tech/PinglyMotion/issues/17) and use case 7 in
[PRODUCT.md](../PRODUCT.md). Sources are linked in each section. Licence texts were read from the
projects' own `LICENSE` files where possible; prices and service terms were read from the vendors'
pages on 20 November 2026 and change often.

> **Not legal advice.** Every licence, copyright and privacy point below is a reading of public
> texts by a non-lawyer. Before anything is bundled or a user is told "you may sell this", the
> licence holder's own text decides, and an unclear case should be asked of the licensor.

## Short answer

- **Do now:** keep video capture *outside* PinglyMotion and make the import side excellent:
  a guide, ready-made joint mappings for the common tools' BVH files, and clean-up tools
  (smoothing, foot lock, key reduction) that help every kind of motion capture.
- **Not now:** a bundled model. The models that give good results (WHAM, GVHMR, TRAM, PromptHMR,
  GENMO) are either under non-commercial licences or need the SMPL body model, whose free licence
  forbids making artefacts for commercial purposes. Second Life animations are often sold, so that
  is a dead end for a built-in feature.
- **Maybe later:** a separate, optional helper program (own licence, own process) on a
  commercially usable model, MediaPipe or Meta's SAM 3D Body, but only after a measured test shows
  the quality is worth it.
- **Biggest blocker:** licences, not technology. The commercially clean, light models are the
  weakest; the strong ones are research-only. PinglyMotion is GPLv2-only, and Apache-2.0 code
  (MediaPipe) cannot be linked into it, so any built-in route must be a separate program.

## 1. What Second Life needs from video motion

These are the targets any video route must hit. Most come from PinglyMotion's own format notes
([ANIM_FORMAT.md](../ANIM_FORMAT.md)).

| Need | Why it is hard from video |
|---|---|
| Joint **rotations** on the SL skeleton (classic joints; Bento fingers optional) | Many video models output joint **positions** only (MediaPipe, OpenPose, RTMPose). Rotations, especially the twist of the forearm, upper arm and head, must be solved afterwards and are often wrong. SMPL-based models output rotations directly, which is why they retarget better. |
| A sensible **root** (pelvis) path | Camera-space models do not know where the person is in the world; walking clips drift or jump. "World-grounded" models (WHAM, GVHMR, TRAM) exist exactly for this. |
| **Feet that stay put** | Foot sliding is the most common complaint about video mocap, see the general overview at [Cinevva](https://app.cinevva.com/guides/ai-animation-generators-3d); WHAM adds a contact-aware trajectory step for this reason ([WHAM paper](https://arxiv.org/abs/2312.07531)). Second Life users notice sliding at once in dances and AO walks. |
| Little **jitter** | Per-frame estimators shake. SL interpolates between keys and users already raise frame rate to make curves smooth, which also raises file size ([SL forum thread on a Rokoko + Cascadeur dance](https://community.secondlife.com/forums/topic/496603-workflow-demo-cascadeur-3d-rokoko/)). |
| **≤ 60 s** and **≤ 250,000 bytes** | Hard limits of the upload. PinglyMotion's exporter already refuses over-long animations and removes keys that a straight line explains to fit the byte limit ([ANIM_FORMAT.md](../ANIM_FORMAT.md)). Raw mocap at 30 fps with fingers easily breaks the byte limit (same forum thread: a first export near 2 MB). |
| Sensible **frame rate** | 30 fps is enough for most SL motion; the forum thread found 20 fps choppy and 40 fps acceptable after key reduction. |
| Correct **spine mapping** | Mapping a mocap spine to the Bento extra spine joints (Spine1 to Spine4) left avatars bent after the animation ended; the advice was to drive only the classic torso and chest ([SL forum thread on deforming mocap](https://community.secondlife.com/forums/topic/495271-mocap-animation-deforming-help/)). |
| **Fingers** | Most video services do not track fingers well or at all: Rokoko Vision 3.0 has no finger tracking at launch ([Rokoko Vision](https://www.rokoko.com/products/vision)); DeepMotion charges extra for hands ([CG Channel on Animate 3D 5.0](https://www.cgchannel.com/2023/10/deepmotion-launches-deepmotion-animate-3d-5-0/)). For SL, hand poses can come from PinglyMotion's own hand presets instead. |

### Skeleton mapping SMPL to SL (sketch)

SMPL has 24 joints: pelvis, three spine joints, neck, head, collars, shoulders, elbows, wrists,
hands, hips, knees, ankles and feet. That maps well onto the classic SL joints:

| SMPL | SL |
|---|---|
| pelvis | `mPelvis` (root, with position) |
| spine1, spine2 / spine3 | `mTorso`, `mChest` (one SMPL spine joint folded into its neighbour; leave Bento `mSpine1`..`mSpine4` alone) |
| neck, head | `mNeck`, `mHead` |
| left/right collar, shoulder, elbow, wrist | `mCollarLeft/Right`, `mShoulderLeft/Right`, `mElbowLeft/Right`, `mWristLeft/Right` |
| left/right hip, knee, ankle, foot | `mHipLeft/Right`, `mKneeLeft/Right`, `mAnkleLeft/Right`, `mFootLeft/Right` |
| hands (SMPL-X / MANO fingers) | Bento finger joints, optional |

PinglyMotion already has the machinery for this: BVH import from any skeleton with a hand-made,
remembered joint map ([RETARGET_MAPPING.md](../RETARGET_MAPPING.md), issue 16) and glTF import
(issue 12). A video tool that writes BVH or glTF needs **no new import code**, only a mapping and
clean-up.

## 2. Research models (2023 to 2026)

| Model | What it gives | Code licence | Weights / body model | Can the output be sold? |
|---|---|---|---|---|
| **WHAM** (CVPR 2024) | World-grounded SMPL motion from video, foot-contact-aware ([paper](https://arxiv.org/abs/2312.07531), [code](https://github.com/yohanshin/WHAM)) | MIT | Needs registration for SMPL and SMPLify; downloads trained models | **No without a commercial SMPL licence** (see below) |
| **TRAM** (2024) | World-grounded SMPL motion with masked DROID-SLAM ([code](https://github.com/yufu-wang/tram)) | MIT | Needs SMPL and SMPLify registration; third-party models "have their own licenses" | **No without a commercial SMPL licence** |
| **GVHMR** (SIGGRAPH Asia 2024) | World-grounded SMPL-X, estimated per frame in a gravity-view frame ([paper](https://arxiv.org/abs/2409.06662v1), [code](https://github.com/zju3dv/GVHMR)) | Educational, research and non-profit only; derivatives must stay non-commercial ([LICENSE](https://github.com/zju3dv/GVHMR/blob/main/LICENSE)) | Needs SMPL and SMPL-X | **No** |
| **4DHumans / HMR 2.0** (2023) | Per-frame SMPL plus tracking ([code](https://github.com/shubham-goel/4D-Humans)) | MIT | SMPL | **No without a commercial SMPL licence** |
| **PromptHMR** (2025) | Promptable SMPL-X recovery ([code](https://github.com/yufu-wang/PromptHMR)) | "Non-Commercial Scientific Research Use Only", explicitly forbids artefacts for commercial purposes ([LICENSE](https://github.com/yufu-wang/PromptHMR/blob/main/LICENSE)) | SMPL-X | **No** |
| **GENMO** (NVIDIA, 2025) | Unified motion estimation and generation ([code](https://github.com/NVlabs/GENMO)) | NVIDIA OneWay Noncommercial License | | **No** |
| **NLF** (Neural Localizer Fields, 2024) | Any-body-model pose and shape ([code](https://github.com/isarandi/nlf)) | MIT | Models released "for noncommercial research use" (README) | **No** |
| **MotionBERT** (2023) | 2D-to-3D lifting of keypoint sequences ([code](https://github.com/Walter0807/MotionBERT)) | Apache-2.0 | Weights trained on Human3.6M and other research datasets; terms of the data not checked | Unclear; positions only |
| **SAM 3D Body** (Meta, Nov 2025) | Single-image full-body mesh incl. hands and feet on the Momentum Human Rig ([code](https://github.com/facebookresearch/sam-3d-body)) | SAM License: royalty-free right to use, copy, modify and distribute, with trade-control and acceptable-use terms ([LICENSE](https://github.com/facebookresearch/sam-3d-body/blob/main/LICENSE)) | Weights under the same SAM License; the rig, [MHR](https://github.com/facebookresearch/MHR), is Apache-2.0 | **Apparently yes** (no non-commercial clause found); per image, not per video, so it needs temporal smoothing; large model, GPU in practice |
| **MediaPipe Pose Landmarker** (BlazePose GHUM 3D) | 33 landmarks with 3D "world" coordinates in metres, hip-centred; lite, full and heavy models; video mode ([guide](https://developers.google.com/edge/mediapipe/solutions/vision/pose_landmarker)) | Apache-2.0 ([LICENSE](https://github.com/google-ai-edge/mediapipe/blob/master/LICENSE)) | A third-party ONNX conversion states code and weights are Apache-2.0 ([Hugging Face](https://huggingface.co/fernandotonon/QtMeshEditor-poselandmarks-onnx)); Google's model card not read (download failed) | **Yes** as far as read; positions only, hands are four points each, root is hip-centred (no world path) |
| **RTMPose / RTMW** (MMPose, 2023 to 2024) | 2D whole-body keypoints incl. hands and feet; RTMW also explores 3D ([paper](https://arxiv.org/abs/2407.08634)); light ONNX runtime wrapper [rtmlib](https://github.com/Tau-J/rtmlib) runs on CPU | Apache-2.0 ([MMPose](https://github.com/open-mmlab/mmpose/blob/main/LICENSE), rtmlib) | Trained on "multi-domain combined datasets"; the datasets' own terms not checked | Probably, with the dataset caveat; 2D only, needs a lifter |
| **OpenPose** | 2D keypoints | Academic or non-profit, non-commercial research only ([LICENSE](https://github.com/CMU-Perceptual-Computing-Lab/openpose/blob/master/LICENSE)) | | **No** |
| **HaMeR** (hands, 2024) | 3D hands ([code](https://github.com/geopavlakos/hamer)) | MIT | Needs the MANO hand model (registration, MPI licence) | **No without a commercial MANO licence** |
| **WiLoR** (hands, 2025) | 3D hands ([code](https://github.com/rolpotamias/WiLoR)) | CC BY-NC-ND 4.0 | | **No** |

### The SMPL catch

Almost every strong 2024 to 2026 model above predicts SMPL or SMPL-X parameters. The free
SMPL-X model licence limits use to non-commercial research, education or artistic projects and
prohibits the "production of other artifacts for commercial purposes"
([SMPL-X model licence](https://smpl-x.is.tue.mpg.de/modellicense.html)); the SMPL model licence
says the same and also forbids using it to train networks for commercial use
([SMPL model licence](https://smpl.is.tue.mpg.de/modellicense.html)). Commercial licences go
through Meshcapade, which holds an exclusive licence to sub-license SMPL
([Meshcapade](https://meshcapade.com/smpl), [SMPL FAQ](https://smpl.is.tue.mpg.de/faq.html)).
The SMPL site also has a separate "SMPL-Body" page under CC BY 4.0
([SMPL-Body licence](https://smpl.is.tue.mpg.de/license.html)) that mentions patents on the model
specification; which files it covers, and whether it would free a video pipeline, was **not**
settled here and would need the licensor's answer.

A Second Life creator who sells a dance made this way would, on this reading, be outside the free
licence. PinglyMotion must not ship or recommend such a pipeline for selling. A user may still run
these tools themselves for private, non-commercial work; that is their decision under the tools'
own terms.

## 3. Open-source tools users can run today

| Tool | What it is | Licence | Output use |
|---|---|---|---|
| **XR Animator** | Webcam or video mocap in the browser or as a desktop app, built on MediaPipe; records and exports BVH, glTF or VMD; body, hands and face; 20+ fps on an entry-level PC ([project](https://github.com/ButzYung/SystemAnimatorOnline)) | Source CC BY-NC-SA 4.0 when adapted | The project states it claims no rights over content made with it |
| **Mocapy** | Video or webcam to BVH with MediaPipe | MIT (see [OPEN_SOURCE_CHECK.md](../OPEN_SOURCE_CHECK.md)) | External tool, BVH goes through our import |
| **FreeMoCap** | Multi-webcam capture | AGPL-3.0; already rejected for code ([OPEN_SOURCE_CHECK.md](../OPEN_SOURCE_CHECK.md)) | External tool only |
| **QtMeshEditor** | Qt desktop editor whose "Performance Capture" downloads a MediaPipe pose model as ONNX on first use and solves body pose with IK ([model page](https://huggingface.co/fernandotonon/QtMeshEditor-poselandmarks-onnx)) | Not checked | A useful precedent: a Qt app that loads the model as a separate data file at run time |

## 4. Commercial services

Prices as listed on 20 November 2026; check before relying on them.

| Service | Price and limits | Export | Commercial use of output | Notes |
|---|---|---|---|---|
| **Rokoko Vision 3.0** | Free: 30 s of processing a month, FBX only. Basic: USD 10/month yearly (12 monthly), 600 s a month, adds BVH ([page](https://www.rokoko.com/products/vision)) | FBX; BVH on paid plan | Terms of use linked but not read | Single camera only now; dual camera being retired; no finger tracking at launch |
| **DeepMotion Animate 3D** | Free: 60 credits a month (1 credit = 1 s body; hands and face +0.5 each), clips up to 20 s, 30 fps; paid from USD 15/month ([CG Channel, 2023](https://www.cgchannel.com/2023/10/deepmotion-launches-deepmotion-animate-3d-5-0/), [Cinevva, 2026](https://app.cinevva.com/guides/ai-animation-generators-3d)) | FBX, BVH, GLB | **Free plan is non-commercial**; paid plans include commercial use | The pricing page itself could not be fetched here |
| **Move One** (Move AI) | Free: 30 one-time credits, videos up to 30 s. Paid from USD 18/month; up to 60 s per video; 1 or 2 credits per second per person ([pricing doc](https://docs.move.ai/knowledge/move-one-pricing)) | Not stated on that page | Not stated on that page | iPhone app |
| **Plask** | Free: 15 s a day; Standard USD 18/month yearly (10 min); Pro USD 50/month (1 h) ([pricing](https://plask.ai/pricing)) | FBX, GLB, BVH on all plans | Not stated on the pricing page | Browser |
| **Radical** | **Closing:** Autodesk bought the technology; the portal shuts and users had until 6 July 2026 to download data ([CG Channel](https://www.cgchannel.com/?p=175200)) | | | Shows the risk of depending on one vendor |

What SL creators actually do today, from the forum threads found: capture with a suit or service
(Rokoko, Xsens), retarget in Blender with Avastar or Bento Buddy, and export `.anim` or BVH. A
creator found Rokoko's feet unreliable for dances and switched to using mocap only for key poses
and Cascadeur for the in-between motion
([thread](https://community.secondlife.com/forums/topic/495056-my-first-mocap-anim/)). File size
and finger keys were the main export pain
([thread](https://community.secondlife.com/forums/topic/496603-workflow-demo-cascadeur-3d-rokoko/)).
Linden Lab's experimental Puppetry project had a webcam plug-in, but streams live joint targets
and does not record animations; it is limited to preview regions
([How Puppetry Works](https://wiki.secondlife.com/wiki/How_Puppetry_Works)). The paid add-ons and
the Blender step are exactly the gap PinglyMotion targets.

## 5. Compute

Estimates from the projects' setup notes, not measured here.

- **MediaPipe and RTMPose/rtmlib:** made for phones and CPUs. Run on an ordinary Windows PC
  without a dedicated GPU; XR Animator reports 20+ fps on an entry-level PC.
- **WHAM, TRAM, GVHMR, 4DHumans, PromptHMR:** PyTorch with CUDA; ViT-sized backbones and often a
  SLAM step (DROID-SLAM in TRAM, DPVO optional in GVHMR, which now defaults to a lighter visual
  odometry). In practice an NVIDIA GPU with several GB of memory and a large Python install.
  GVHMR's checkpoint was trained on two RTX 4090 cards; inference needs far less but still a GPU.
- **SAM 3D Body:** large ViT/DINOv3 backbones; a GPU in practice, and per image, so a 60 s clip at
  30 fps is 1,800 runs.
- **Cloud:** the services above do the GPU work for a fee. Running our own cloud would cost money
  every month, need an operator, and make PinglyMotion responsible for users' videos (section 7).

## 6. Licence of PinglyMotion itself

PinglyMotion is GPLv2-only ([README](../../README.md)). The Apache Software Foundation states that
the FSF has never considered Apache-2.0 compatible with GPLv2, because of its patent termination
and indemnity terms ([ASF on GPL compatibility](https://apache.org/licenses/GPL-compatibility.html)).
So MediaPipe, MMPose or MHR code cannot be compiled or linked into PinglyMotion.

Two ways that are commonly treated as acceptable (not legal advice):

- **A separate program** that PinglyMotion starts and talks to over files, pipes or a local
  socket, as independent programs normally do. The GPL FAQ, as summarised by
  [ifrOSS](https://www.ifross.org/en/when-does-independently-developed-software-have-be-licensed-under-gpl),
  treats such programs as separate unless the coupling is close. This is the "small helper
  program" idea already in issue 17. The GPL FAQ itself could not be fetched in this session.
- **Model weights as a separate data file**, loaded at run time by a GPL-compatible runtime
  (ONNX Runtime is MIT). PinglyMotion already ships AGPL Ruth2 and Roth2 meshes as separate data
  files on the same reasoning ([README](../../README.md)). Whether an Apache-2.0 weights file
  loaded into a GPLv2 process is "data" is a grey zone; the helper program avoids the question.

## 7. Privacy of user videos

- A video of a person's body and face is personal data under European rules (GDPR applies in the
  EEA). PinglyMotion should never upload a user's video anywhere itself.
- With a commercial service the user uploads to that vendor under the vendor's terms (storage,
  retention, possible use for training). Those terms were not read in this research; the guide
  should tell users to read them and to prefer local tools for private footage.
- Videos of other people (a friend, a music video) need their consent; a famous dancer's
  choreography may also be protected. PRODUCT.md already says ownership of imported material must
  be checked separately; the guide should repeat it.
- A local helper keeps the video on the user's machine and needs no account.

## 8. Integration options for PinglyMotion

| Option | Quality | Licence risk | Effort | Verdict |
|---|---|---|---|---|
| **(a) External tool, then BVH/glTF import** (works today) | Whatever the tool gives; paid services give the best | None for PinglyMotion; the user follows the tool's terms | Small: guide, joint maps, clean-up tools | **Do first** |
| **(b) Bundled local model** | MediaPipe level (positions, no world path, weak twist) unless SAM 3D Body proves usable | Must be a separate program; strong models are non-commercial | Large: helper build for Windows, packaging, rotation solver, smoothing | **Spike only, later** |
| **(c) Optional cloud service** | Good | Vendor terms; privacy; cost; vendor may close (Radical) | Large, ongoing cost | **No** for a free project; users already have (a) |
| **(d) MCP / assistant route** | Depends on (a) or (b) | As (a) | Small once the control commands can import | **Useful helper**: an assistant can run the user's chosen tool, import the BVH, then use `describe_motion` / `check_animation` (issue 129) to find sliding feet and jitter and fix them |

## 9. Recommendation

1. **Route (a) as the official way**, with a short guide "From video to Second Life" naming free
   and paid tools neutrally (XR Animator, Rokoko Vision, DeepMotion, Plask, Move One), their
   free-plan limits on commercial use, and the privacy and rights points.
2. **Make PinglyMotion the best clean-up step**: the features that turn rough mocap into a good SL
   animation are the same whatever the source: smoothing, foot lock, key reduction to fit
   250,000 bytes, root handling, and dropping the Bento spine. This is where PinglyMotion beats
   "Blender plus a paid add-on".
3. **Keep a bundled model as a measured experiment**, never as a promise. If done, it is a separate
   helper program under its own licence on MediaPipe or SAM 3D Body, local only, optional download.
   Never SMPL-based models in anything we ship.

## 10. Risks

- **Licences change or are misread.** Re-check every component's own text before bundling; SAM
  License terms and the SMPL-Body page deserve a direct question to the licensor if ever relied on.
- **Quality disappointment.** Users expect "upload a TikTok, get a perfect dance". Monocular video
  loses depth; fast spins, floor work and hands fail. The guide must set expectations.
- **Vendor churn.** Radical closed in 2026; prices and free tiers move. The guide should date its
  facts.
- **Rights in the source video.** Users may capture someone else's performance and sell it.
- **Scope creep.** A video helper with Python, PyTorch and GPU drivers would dwarf the editor;
  keep it out of the main program.

## 11. Next jobs (proposals; each waits for the project owner's "start")

### Job 1: Guide and joint maps for video-mocap BVH (docs and data, technical)

- **What:** `docs/VIDEO_GUIDE.md` (tools, limits, privacy and rights, step by step to `.anim`) and
  bundled `.jointmap` presets for the BVH skeletons of XR Animator and of one or two services'
  default BVH skeletons, found from their documentation.
- **Acceptance tests:** for each preset, a small generated BVH with exactly that hierarchy (no
  third-party motion data) imports with every classic SL joint matched, no Bento spine joint
  driven, and the list of left-out joints exactly as the test expects; the guide passes
  `tools/check_texts.py` and links only to vendor pages.

### Job 2: Mocap clean-up core (technical first, menu later)

**State (20 November 2026):** the core is built in `src/motioncleanup.*` with tests in
`tests/motioncleanup/`: resample to 30 fps, start pose at frame 1, in place, loop seam blend and
light quaternion-safe zero-phase smoothing (binomial, the first and last frame kept). The smoothing
test uses the CMU walk with generated noise of up to 2 degrees: the shake drops below a third and
to within 1.2 times the clean clip's own, and the worst joint stays within 2 degrees of the clean
clip as root mean square over the frames. Foot lock is built in `src/footlock.*` with tests in
`tests/footlock/`: a foot is planted while its ankle is less than 2 inches (5 cm, as in
`describe_motion` and `check_animation`) above the floor (the lowest ankle of the clip) and slower than 0.2 m/s for at least 3 frames; the ankle is pinned at the mean
of where it was, the thigh and shin are solved with an exact two-bone IK in the knee's own plane,
the foot keeps its turn in the world, and the pin is blended in and out over 4 frames. A pin the
leg cannot reach (the hip has moved on at the end of a step) is followed only as far as the leg is
long. With the joint limits of `bin/data/SL.lim` the planted feet of the CMU walk drift at most
0.62 cm (2.48 cm before); a clip with no contact, every joint but the legs, and all frames away
from a contact are unchanged. One call for the import dialog: `FootLock::lockFeet()`. Not built
yet: control commands and the menu.
The 250,000 byte limit is the `.anim` exporter's job (key reduction, already built).

- **What:** functions for zero-phase smoothing of rotations (quaternion-safe), foot lock (detect
  contact from low foot speed and height, pin the foot, correct the leg) and root clean-up, with
  control commands so an assistant can call them. The menu entries come in a separate visible job.
- **Acceptance tests:** on a bundled CMU clip with added generated noise, the frame-to-frame
  angular jitter after smoothing is below a set threshold and the clip's key poses stay within
  2 degrees of the original; on a CMU walk, foot-lock keeps each planted foot within 1 cm for the
  contact frames; a 60 s clip at 30 fps with all joints exports below 250,000 bytes; round trip
  through `.dingle` is unchanged.

### Job 3: Spike: local helper on a commercially usable model (research)

- **What:** outside the main build, measure MediaPipe Pose (ONNX), rtmlib whole-body plus a lifter,
  and SAM 3D Body on three openly licensed video clips: CPU speed on a CI Windows runner, jitter,
  foot sliding against a reference where one exists, and licence of every file used.
- **Acceptance tests:** a report with a measured table per model (fps, jitter, foot drift), each
  component's licence quoted from its own file, and a yes or no on whether a helper is worth
  building; no code merged into the program.

## Not checked

Google's BlazePose model card terms; Rokoko, DeepMotion, Move AI and Plask terms of use on output
ownership and training on uploads; the datasets behind RTMPose and MotionBERT weights; Apple's
CoMotion licence; the full GPL FAQ text; real quality measurements of any model.
