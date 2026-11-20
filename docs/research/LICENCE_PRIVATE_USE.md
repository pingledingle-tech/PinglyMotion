# Licence check: AI motion tools for private, non-selling use

Date: 20 November 2026. Scope: every model, tool, dataset and service named in
`docs/research/VIDEO_TO_MOTION.md` and `docs/research/TEXT_TO_MOTION.md`, re-checked for a
**different use** than those reports assumed: the user will **not sell** anything.

> **Not legal advice.** This is a plain reading of public licence texts on the date above.
> Licences change; several texts are ambiguous, and only the licensor (or a lawyer) can settle
> an ambiguous case. "Yes" means "the text read here appears to allow it", not a guarantee.

## The three cases

1. **Private:** run on the user's own PC, animation used only by the user (no copy given to anyone).
2. **SL gift:** upload the result to Second Life (the user pays the L$ upload fee) and give copies free to friends.
3. **Repo:** commit the model's output (BVH, `.anim`, test data, examples) to the public GPL PinglyMotion repository.

Territory: Norway is in the EEA but not the EU. That matters only for HY-Motion.

## Cross-cutting findings

**A. Second Life's own Terms of Service are the real hurdle for case 2, not the money.**
No licence read here treats paying an upload fee as commercial use. But SL ToS section 2.3 (https://lindenlab.com/tos) has every uploader grant Linden Lab a perpetual, irrevocable, worldwide licence that includes the right to "sell, re-sell or sublicense", and the uploader warrants the uploader has all rights needed (2.3, 1.3; 2.7 says you must get the needed licences yourself). So for case 2 the question is: does the tool's licence let the uploader grant that licence over the output? Where a tool claims no rights in outputs (NVIDIA Open Model License, Tencent HY-Motion, XR Animator, Apache tools) that is fine. Where a licence says outputs stay under non-commercial or no-third-party terms (PromptHMR, free plans of some services), the uploader cannot honestly give that warranty. For the SMPL family the text says nothing about outputs, so it stays unclear.

**B. "Non-commercial artistic projects" vs "research only".** The Max Planck family (SMPL, SMPL-X, SMPL-H via the MANO site, MANO, AMASS) and PromptHMR allow non-commercial research, education or non-commercial artistic projects. Animations for one's own avatar fit "artistic project". Licences that say only "research" or "research or evaluation" (GENMO, OpenPose, NLF weights, MotionLCM, NVIDIA R&D licence) do not define research; hobby animation is not obviously research, so those stay unclear even for private use. None of the texts mentions games or virtual worlds. Several forbid pornographic use whether paid or not (SMPL family, AMASS, MANO, PromptHMR, MotionLCM): that covers adult animations in SL too.

**C. Case 3 is stricter than case 2.** Anything committed to a GPL repo is offered to the whole world under terms that allow commercial reuse by others. So any output under a non-commercial, no-third-party or territorial limit is a No for case 3, even where case 2 is arguable.

## 1. Video to motion: models

| Item | 1 Private | 2 SL gift | 3 Repo | Licence read |
|---|---|---|---|---|
| WHAM | Yes. Code MIT; needs SMPL + SMPLify (registration), which allow non-commercial artistic use. | Unclear. MIT fine; SMPL text says nothing about outputs; SL ToS warranty (A). | No. SMPL data may not be made available to third parties; GPL invites commercial reuse. | https://github.com/yohanshin/WHAM/blob/main/LICENSE ; https://smpl.is.tue.mpg.de/modellicense.html |
| TRAM | Yes. Same as WHAM. | Unclear. Same. | No. Same. | https://github.com/yufu-wang/tram/blob/main/LICENSE |
| GVHMR | Yes. "educational, research and non-profit purposes"; hobby is non-profit. Plus SMPL/SMPL-X. | Unclear. SMPL-X chain and SL ToS (A) open. | No. Derivatives must be non-commercial; GPL conflicts. | https://github.com/zju3dv/GVHMR/blob/main/LICENSE |
| 4DHumans / HMR 2.0 | Yes. MIT + SMPL (registration). | Unclear. Same as WHAM. | No. | https://github.com/shubham-goel/4D-Humans/blob/main/LICENSE |
| PromptHMR | Yes. Grant includes "non-commercial artistic projects". | No. Artifacts stay under the same terms and may not be made available to third parties without written permission. | No. | https://github.com/yufu-wang/PromptHMR/blob/main/LICENSE |
| GENMO (NVIDIA) | Unclear. NVIDIA OneWay Noncommercial: "research or evaluation purposes only". | No (leaning). Plus SL ToS (A). | No. | https://github.com/NVlabs/GENMO/blob/main/LICENSE |
| NLF | Unclear. Code MIT, weights "for noncommercial research use" (README only). | Unclear. | No. | https://github.com/isarandi/nlf |
| SAM 3D Body (Meta) | Yes. SAM License: royalty-free, no non-commercial clause; weights gated. | Yes. Licence limits only SAM Materials and derivatives, not outputs; trade-control rules only. | Yes. Outputs are not SAM Materials. (Shipping weights would need the licence copy.) | https://github.com/facebookresearch/sam-3d-body/blob/main/LICENSE |
| MediaPipe Pose | Yes. Code Apache-2.0; BlazePose model card states Apache-2.0. | Yes. | Yes. | https://github.com/google-ai-edge/mediapipe/blob/master/LICENSE |
| RTMPose / RTMW | Yes. Apache-2.0 (MMPose, rtmlib). | Yes (training-data terms not checked; 2D only, needs a lifter). | Yes, same caveat. | https://github.com/open-mmlab/mmpose/blob/main/LICENSE |
| OpenPose | Unclear. Only "noncommercial internal research". | No (leaning). | No. | https://github.com/CMU-Perceptual-Computing-Lab/openpose/blob/master/LICENSE |
| HaMeR (hands) | Yes. MIT + MANO (registration, artistic use allowed). | Unclear. MANO silent on outputs; SL ToS (A). | No. MANO data may not go to third parties. | https://github.com/geopavlakos/hamer/blob/main/LICENSE.md ; https://mano.is.tue.mpg.de/license.html |
| WiLoR (hands) | Yes. Models CC BY-NC-ND 4.0 + MANO. | Unclear. CC covers the model, not normally outputs; MANO chain and SL ToS open. | No. | https://github.com/rolpotamias/WiLoR/blob/main/license.txt |
| XR Animator | Yes. Built on MediaPipe; claims no right over content made with it. | Yes. CC BY-NC-SA applies only to adapting its source code. | Yes for its output; not for copying its source into the GPL repo. | https://github.com/ButzYung/SystemAnimatorOnline |

## 2. Text to motion: models

| Item | 1 Private | 2 SL gift | 3 Repo | Licence read |
|---|---|---|---|---|
| MoMask | Yes. Code MIT; weights trained on HumanML3D/AMASS (non-commercial artistic use allowed). | Unclear (leaning yes). AMASS has no rule on outputs of trained models; SL ToS (A); no adult content. | Unclear, avoid. | https://github.com/EricGuo5513/momask-codes/blob/main/LICENSE ; https://amass.is.tue.mpg.de/license.html |
| MDM | Yes. Same as MoMask. | Unclear (leaning yes). | Unclear, avoid. | https://github.com/GuyTevet/motion-diffusion-model/blob/main/LICENSE |
| MotionGPT | Yes. Same as MoMask. | Unclear (leaning yes). | Unclear, avoid. | https://github.com/OpenMotionLab/MotionGPT/blob/main/LICENSE |
| MotionLCM | Unclear. Says "non-profit purposes" but grant is "solely for your non-commercial research purposes". | Unclear. Also bans pornographic use. | No. | https://github.com/Dai-Wenxun/MotionLCM/blob/main/LICENSE |
| HY-Motion 1.0 (Tencent) | Yes, on the literal text. Territory is "worldwide ... excluding the territory of the European Union, United Kingdom and South Korea": Norway not named. Caveat: Tencent's reason not stated; Norway applies much EU regulation via EEA; governing law Hong Kong. | No (in practice). Must not use/reproduce/modify/distribute/display Output outside the Territory (5c). In SL, friends and bystanders in the EU would receive or see it. Use policy asks to label machine-generated content in public. | No. A public repo makes Output available in the EU. | https://github.com/Tencent-Hunyuan/HY-Motion-1.0/blob/main/LICENSE |
| Kimodo-SOMA (RP or SEED models) | Yes. NVIDIA Open Model License: "Models are commercially usable", worldwide. Text encoder needs gated Meta Llama 3 8B Instruct (accept Meta's licence on Hugging Face, token). | Yes. NVIDIA claims no ownership of outputs; Llama 3 outputs here are internal embeddings. | Yes. Do not commit the weights. | https://github.com/nv-tlabs/kimodo ; https://www.nvidia.com/en-us/agreements/enterprise-software/nvidia-open-model-license/ ; https://www.llama.com/llama3/license/ |
| Kimodo-SMPLX | Unclear. NVIDIA R&D licence: internal scientific R&D, non-production only. | No. Bars generating works "for sale or distribution". | No. | https://www.nvidia.com/en-us/agreements/enterprise-software/nvidia-internal-scientific-research-and-development-model-license/ |

Kimodo guardrails: the Open Model License ends automatically if you bypass the model's guardrails; NVIDIA's Trustworthy AI terms forbid unlawful, deceptive or harassing use.

## 3. Body models and datasets

| Item | 1 Private | 2 SL gift | 3 Repo | Licence read |
|---|---|---|---|---|
| SMPL | Yes. Registration; "non-commercial artistic projects" allowed. | Unclear. Model may not be made available to third parties; whether a BVH of joint rotations is "the Software" not addressed; SL ToS (A). | No for model files; avoid for derived motion. | https://smpl.is.tue.mpg.de/modellicense.html |
| SMPL-Body (subset) | Yes. CC BY 4.0 with credit. | Yes, with credit. | Yes, with credit (note patents listed on the page; no shape blendshapes). | https://smpl.is.tue.mpg.de/license.html |
| SMPL-X | Yes. Same wording as SMPL. | Unclear. | No. | https://smpl-x.is.tue.mpg.de/modellicense.html |
| SMPL-H | Yes. Via MANO site after accepting its licence. | Unclear. | No. | https://mano.is.tue.mpg.de/ |
| MANO | Yes. | Unclear. | No. | https://mano.is.tue.mpg.de/license.html |
| AMASS | Yes. Registration; artistic use allowed. | No for raw AMASS motion. Outputs of models trained on it: see MoMask. | No. | https://amass.is.tue.mpg.de/license.html |
| HumanML3D | Yes. Repo MIT, motion from AMASS. | No for its motion data. | No. | https://github.com/EricGuo5513/HumanML3D |
| BONES-SEED | No. Only non-profit academic institutions and startups under USD 1M revenue qualify. | No. | No. | https://bones.studio/info/seed-license |

BONES-SEED's limits bind whoever downloads the dataset. They do not stop the user from using the Kimodo models NVIDIA trained and licensed under its own Open Model License.

## 4. Online services, free tiers

| Service | 1 Private | 2 SL gift | 3 Repo | Terms read |
|---|---|---|---|---|
| DeepMotion Animate 3D (free) | Yes. Freemium is "for personal, non-commercial use". | Unclear (leaning no). "Licensed animation" needs a paid plan; SL ToS (A). | No. | https://www.deepmotion.com/pricing-animate3d |
| SayMotion (DeepMotion) | Unclear. No free-plan terms found. | Unclear. | No. | https://deepmotion.com/pricing-saymotion |
| Rokoko Vision / Create (free) | Yes. | Unclear (leaning yes). You keep rights to your User Content; Rokoko Assets licensed only while you have an account. Free Vision exports FBX only. | Unclear. | https://www.rokoko.com/studio-term-of-use ; https://rokoko.com/pricing/ |
| Plask (free) | Yes. "personal non-commercial use". | Unclear. Output contains Plask IP under a personal, non-transferable licence. | No. | https://plask.ai/docs/100002 |
| Move One (free credits) | Yes. | Unclear. Output licence sublicensable only with written flow-down agreement (7.5.1). | No. | https://www.move.ai/move-one-eula |
| Krikey (free) | Yes, probably. Free plan seems watermarked, no FBX. | Unclear. Terms do not say who owns generated animations. | Unclear, avoid. | https://www.krikey.ai/terms ; https://www.krikey.ai/pricing/ai |

Services also take broad licences to what you upload (Move One and Krikey may use it to train AI). Use only video of yourself or video you have rights to.

## What this opens up for the user
- Private use (case 1) of almost all academic pipelines: WHAM, TRAM, GVHMR, 4DHumans, PromptHMR, HaMeR, WiLoR, MoMask, MDM, MotionGPT, with SMPL/SMPL-X/MANO/AMASS after registration.
- HY-Motion for private use on the user's own PC (territory clause names EU, UK, South Korea only; literal reading).
- Free tiers of DeepMotion, Plask, Rokoko, Move One for private use.
- Fully clean for all three cases: Kimodo-SOMA (with Llama 3 access), SAM 3D Body, MediaPipe, RTMPose, XR Animator. Use these for SL gifts and repo test data.

## What stays off-limits
- Repo (case 3): any output from the SMPL/AMASS family, PromptHMR, GENMO, NLF, OpenPose, MotionLCM, HY-Motion, Kimodo-SMPLX, or free service tiers. Keep test data to the clean list.
- HY-Motion in Second Life: displaying or giving Output to people in the EU or UK is outside its licence, and SL cannot limit who sees it.
- PromptHMR outputs to anyone else, and the BONES-SEED dataset itself.
- Adult animations from anything in the Max Planck family, PromptHMR or MotionLCM, even free.
- SL gifts from the SMPL family and free service tiers stay unclear, because SL's ToS makes the uploader grant Linden Lab a resale-capable licence that these texts may not allow the uploader to give.

Not legal advice. Re-read the licence of the exact version before relying on it.
