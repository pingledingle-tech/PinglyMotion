# PinglyMotion 0.4.0 beta – release notes (draft)

> Draft. Not released yet. Covers everything merged after 0.3.0-beta.1 (15 November 2026), up to 19 November 2026.

## Before you update

- **Your graphics card must support OpenGL 3.3.** Almost every computer from the last ten years does, and Second Life itself needs OpenGL 3.2 or newer. If yours does not, the 3D view stays empty and PinglyMotion tells you once that OpenGL 3.3 is needed.
- **Qt 5 is no longer supported.** This only matters if you build PinglyMotion yourself: you now need Qt 6. The Windows zip and the Linux AppImage already include everything.

## What's new

- **New look for the 3D view.** The avatar is drawn with modern graphics: softer, more natural light, and a soft shadow under the figure on the floor.
- **Light settings.** *Options > Light…* lets you make the light brighter or darker, more matt or shiny, cooler or warmer, and turn the floor shadow on or off. *Reset* goes back to the standard look. Your choice is remembered.
- **See your pose on your own avatar in Aurora.** *Options > Live link to Aurora* shows the pose you are working on, on your avatar in the Aurora viewer, while you work. Only you see it, nothing is uploaded. This needs a test build of Aurora for now – see `docs/AURORA_LINK.md`.
- **Joints sorted in tabs.** The "Edit Part" drop-down is replaced by tabs **Body / Face / Hands / Other** with a list of joints. Click a joint in the 3D view and the list jumps to it. Arrow keys move through the list.
- **Joints with plain names, and a search box.** The joint list says "Forearm" and "Wrist" instead of `lForeArm` and `lHand`, under headings such as *Left Arm* and *Legs*. Rest the mouse on a joint to see its Second Life name. Type in the search box above the list, for example "left knee", to find a joint on any tab.
- **More realistic joint limits.** Joint limits now follow the Second Life wiki's suggested limits instead of the old QAvimator values. Fingers bend, spread and twist within a natural range.
- **Status bar.** Besides the remote control indicator, the status bar shows whether the Aurora live link is off, connecting or live. Click the dot to turn the link on or off. A "Tips" switch is there too; it only stores your choice for now, the tips come later.
- **Remote control indicator.** When an AI assistant (for example Claude) controls PinglyMotion, a coloured dot in the status bar shows it: grey = off, yellow = waiting, green = connected, red = a wrong key was refused. It also shows the assistant's name and its last command. Click the dot to turn remote control on or off.

## What's changed and fixed

- **Unsaved changes:** the question now offers **Save / Don't Save / Cancel**, and Save is the default. Pressing Enter no longer throws your work away.
- **Save As** lets you choose between *Second Life animation (.bvh)* and *PinglyMotion animation (.avm)*, and adds the right file ending for you.
- **Skeleton mode:** the small face joint balls only show when the head or a face joint is selected, so the face is no longer cluttered.
- **Loop points** are kept when you open a file and save it again. A file without loop points no longer picks up the loop points from the file you had open before.
- **IK** (keeping a hand or foot in place while you move the body) has a new solver. Locked hands and feet follow the 3D view closely, an unreachable goal no longer throws an arm or leg around, and a value you type is solved at once.
- **Body choice.** **Options > Body** replaces "Bento body (Max)": pick the open source Ruth2 avatar (the default) or Max. Both have fingers and a face that follow the Bento bones. If you had chosen Max, you keep Max.
- **The Second Life system avatar is gone.** It is replaced by Ruth2. If your settings said "system avatar" (or "Bento body" off), PinglyMotion now shows Ruth2; nothing else changes. The male figure shows the new Roth2 v2 body (see the next item).
- **A real male body.** A male figure now shows Roth2 v2, the male sibling of Ruth2, with fingers and a face that follow the Bento bones. It is chosen by itself from the figure type: there is no new menu entry, and **Options > Body** only changes the female figure. The nails of Roth2 are not included (their licence is non-commercial).
- **Credits.** Help > About and the README name the makers of Ruth2 and Roth2 and give their licence (AGPL for the mesh, kept as a separate data file next to the program).
- **Head and neck** can bend up to 45 degrees forward and back.
- **Opening files** uses a new reader. A broken file is refused with a reason that names the line, for example "Line 12: …". Files with values that make no sense (for example numbers too large for a decimal) and AVM files with extra property data are now refused; an empty file says "The file is empty."
- **Window title** shows the version, where the build came from (pull request or branch), its commit and the build number, from the moment the program starts, so you can tell which build you are testing. Test builds of pull requests showed only "dev" before.
- **Timeline** is visible at every start. Before, closing the program with the window's close button switched the timeline off for the next start. Hiding it now lasts until you close the program.
- **Links inside the program** (About, update notice, release info) point to the PinglyMotion repository.
- The Windows zip and the Linux AppImage are built with the newer Qt 6.12 (long-term support), which includes security fixes.

## Known problems

- **IK can twist the body** when you turn the hip a full turn with a hand and a foot locked. At 359° the pose does not return exactly to the start; typing 0 resets it.
- **The live link only works with a test build of Aurora**, not with the normal Aurora download, and not while Aurora's Blender add-on is connected (one program at a time).
- The new joint tabs only filter the list; the 3D view still shows all joints.

## Upload to Second Life

PinglyMotion saves Second Life `.bvh` files that are checked against the rules of the Second Life uploader, but please test your animation in Second Life (for example on the Beta Grid) before you rely on it.
