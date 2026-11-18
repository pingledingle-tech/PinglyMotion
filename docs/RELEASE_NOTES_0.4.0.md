# Pingmator 0.4.0 beta – release notes (draft)

> Draft. Not released yet. Covers everything merged after 0.3.0-beta.1 (15 November 2026).

## Before you update

- **Your graphics card must support OpenGL 3.3.** Almost every computer from the last ten years does, and Second Life itself needs OpenGL 3.2 or newer. If yours does not, the 3D view stays empty and Pingmator tells you once that OpenGL 3.3 is needed.
- **Qt 5 is no longer supported.** This only matters if you build Pingmator yourself: you now need Qt 6. The Windows zip and the Linux AppImage already include everything.

## What's new

- **New look for the 3D view.** The avatar is drawn with modern graphics: softer, more natural light, and a soft shadow under the figure on the floor.
- **Light settings.** *Options > Light…* lets you make the light brighter or darker, more matt or shiny, cooler or warmer, and turn the floor shadow on or off. *Reset* goes back to the standard look. Your choice is remembered.
- **See your pose on your own avatar in Aurora.** *Options > Live link to Aurora* shows the pose you are working on, on your avatar in the Aurora viewer, while you work. Only you see it, nothing is uploaded. This needs a test build of Aurora for now – see `docs/AURORA_LINK.md`.
- **Joints sorted in tabs.** The "Edit Part" drop-down is replaced by tabs **Body / Face / Hands / Other** with a list of joints. Click a joint in the 3D view and the list jumps to it. Arrow keys move through the list.
- **More realistic joint limits.** Joint limits now follow the Second Life wiki's suggested limits instead of the old QAvimator values. Fingers bend, spread and twist within a natural range.
- **Remote control indicator.** When an AI assistant (for example Claude) controls Pingmator, a coloured dot in the status bar shows it: grey = off, yellow = waiting, green = connected, red = a wrong key was refused. It also shows the assistant's name and its last command. Click the dot to turn remote control on or off.

## What's changed and fixed

- **Unsaved changes:** the question now offers **Save / Don't Save / Cancel**, and Save is the default. Pressing Enter no longer throws your work away.
- **Save As** lets you choose between *Second Life animation (.bvh)* and *Pingmator animation (.avm)*, and adds the right file ending for you.
- **Skeleton mode:** the small face joint balls only show when the head or a face joint is selected, so the face is no longer cluttered.
- **Loop points** are kept when you open a file and save it again. A file without loop points no longer picks up the loop points from the file you had open before.
- **IK** (keeping a hand or foot in place while you move the body) now follows the 3D view much more closely, so locked hands and feet drift far less.
- The Windows zip and the Linux AppImage are built with the newer Qt 6.12 (long-term support), which includes security fixes.

## Known problems

- **IK can throw an arm or leg around** when the hand or foot cannot reach its locked spot, for example when you turn the hip very far. ([#127](https://github.com/pingledingle-tech/Pingmator/issues/127))
- **IK is one step behind** when you type a value or make a big jump: the pose is solved for the previous value. Small drags hide it. ([#128](https://github.com/pingledingle-tech/Pingmator/issues/128))
- **Head and neck do not bend far enough** forward and backward with the new joint limits. ([#129](https://github.com/pingledingle-tech/Pingmator/issues/129))
- **The live link only works with a test build of Aurora**, not with the normal Aurora download, and not while Aurora's Blender add-on is connected (one program at a time).
- The new joint tabs only filter the list; the 3D view still shows all joints.

## Upload to Second Life

Pingmator saves Second Life `.bvh` files that are checked against the rules of the Second Life uploader, but please test your animation in Second Life (for example on the Beta Grid) before you rely on it.
