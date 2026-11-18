# Ideas for Pingmator

This is a place to keep possibilities, not a promise that every idea will be built. For the product vision, user value, use cases and proposed roadmap, see [PRODUCT.md](PRODUCT.md). Existing issues and pull requests track detailed requirements and current work.

The goal is to make Second Life animation approachable and free, including AO stands, sits and dances, without requiring professional software or paid add-ons.

## Motion from ordinary video

Explore free, open source tools that estimate motion from ordinary video, so someone could start a dance without a mocap suit. Adapting the motion to Bento, handling missing depth and cleaning up the result need investigation. **Status: idea.**

## Mesh2Motion

Investigate [Mesh2Motion](https://github.com/Mesh2Motion/mesh2motion-app) as a source of animations and inspiration. Its motion uses a different skeleton, so SL Bento retargeting is needed. Check the licence and provenance of each asset and any code before reuse or distribution. **Status: idea.**

## A free bridge to Second Life

Explore importing or converting animation from Blender, Maya, Mesh2Motion and other tools, fitting it to the SL Bento skeleton and exporting for Second Life. Skeleton mapping, rest poses and SL upload behaviour must be validated; an import format alone does not solve retargeting. glTF import and Blender workflows are tracked in [#12](https://github.com/pingledingle-tech/PinglyMotion/issues/12) and [#13](https://github.com/pingledingle-tech/PinglyMotion/issues/13). **Status: planned for later.**

## Preview on an avatar in Aurora

The live link over Aurora's LiveMesh works (old #78, quaternion poses in old #95); see [AURORA_LINK.md](AURORA_LINK.md). It is local to the user and does not upload an animation, and it needs the Aurora fork for now. Further ideas: synchronised playback, bringing avatar shape into Pingmator, and proposing the link to the Aurora developers. **Status: preview done; the rest are ideas.**

## Pose and animation workflow

Explore easier body/face/hand posing, hand presets, mirroring, a clearer timeline, interpolation and easing, seamless looping and reusable motion sequences. BPM guides could help with dances. See [#14](https://github.com/pingledingle-tech/PinglyMotion/issues/14) and [#11](https://github.com/pingledingle-tech/PinglyMotion/issues/11). **Status: proposed work.**

## AI-assisted creation

Build on the existing [local control interface](CONTROL.md) so an assistant can help create and refine poses and sequences. Explore feedback from joint data and screenshots; more ambitious visual assessment and automatic dance creation need research. Keep manual workflows available. **Status: basic controls exist; extensions are ideas.**

## Own AO and creator tools

Create an AO using original animations, potentially for sale. Explore reusable pose/motion libraries and simple LSL controller templates for furniture and poseballs. Check rights for any imported assets; creating an animation in Pingmator alone does not grant rights to its source material. **Status: ideas.**

## Keeping ideas useful

Add the user problem, expected value, dependencies and an issue link when an idea becomes concrete. The project owner decides priorities; no ordering here is a delivery commitment.
