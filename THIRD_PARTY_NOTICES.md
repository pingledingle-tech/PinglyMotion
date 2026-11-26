# Licence and notices

This file comes with every PinglyMotion package (the Windows zip, the Linux AppImage and the MCP extension). It says under which licence PinglyMotion is shared, and it holds the notices that the licences of other people's work ask for. It is not legal advice. The full list of what PinglyMotion uses and learned from is in `CREDITS.md`. The licence texts are in the folder `licenses/` next to this file.

## PinglyMotion

PinglyMotion is free software. You may use, study, share and change it under the GNU General Public License, version 2 or later.

PinglyMotion is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. It is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

The text of version 2 is in `LICENSE` and in `licenses/GPL-2.0.txt`. PinglyMotion is based on QAvimator (Zi Ree and the QAvimator team), itself a Qt port of Avimator, which is also GPL 2 or later.

PinglyMotion is an independent project. It is not made, endorsed or supported by Linden Research, Inc. (Linden Lab). Second Life and Linden Lab are trademarks of Linden Research, Inc. Firestorm, Aurora, OpenCollar and other names belong to their owners.

## What is inside the packages, and under which licence

### Qt 6

- Used for the windows, the 3D view and the network code.
- Licence: LGPL version 3 (with GPL version 3). Texts: `licenses/LGPL-3.0.txt`, `licenses/GPL-3.0.txt`.
- How it is linked: as shared libraries. In the Windows zip they are the `.dll` files and the `platforms`, `styles` and similar folders next to `PinglyMotion.exe`. In the AppImage they are the `.so` files inside it. They are not built into the program file, so you can replace them with another build of Qt.
- Qt is copyright The Qt Company Ltd. and other contributors. Source code and licence terms: https://www.qt.io/licensing and https://download.qt.io. Qt itself contains parts of other projects; their notices are in Qt's own documentation.
- The Windows zip also holds the runtime libraries `libgcc_s_seh-1.dll`, `libstdc++-6.dll` and `libwinpthread-1.dll` from the MinGW-w64 compiler. They come under the GCC Runtime Library Exception and the MinGW-w64 licences.

### Lucide icons

- The icons drawn by the program (the path data is copied unchanged).
- Copyright Lucide Icons and Contributors. Licence: ISC. Some icons are derived from Feather (copyright Cole Bemis, MIT). Both texts: `licenses/Lucide-ISC-and-Feather-MIT.txt`.

### Second Life lsl-definitions and viewer keyword list

- Names, types and values of LSL functions, events and constants, and the first sentence of each description, inside the program (Script Lab).
- `lsl-definitions`: copyright 2025 Linden Research, Inc. Licence: MIT. Text: `licenses/Linden-lsl-definitions-MIT.txt`.
- The keyword list of the Second Life viewer and of the Firestorm viewer (names, types and values only): LGPL version 2.1. Text: `licenses/LGPL-2.1.txt`.

### Firestorm viewer data

- The Bento skeleton data and the hand presets and joint names in the folder `data/firestorm/`. Licence: LGPL version 2.1 (Linden Research, Inc. and the Firestorm project). Text: `data/firestorm/LICENSE.txt` and `licenses/LGPL-2.1.txt`. Credit: the Firestorm project, https://github.com/FirestormViewer/phoenix-firestorm.

### Avatar bodies in the folder `data/character/`

- Ruth2 v4 (`ruth2/ruth2.pmm`) and Roth2 v2 (`roth2/roth2.pmm`), by the RuthAndRoth project (Shin Ingen, Ada Radius and many contributors).
  - The meshes are under the GNU Affero General Public License version 3 or later. The UV map is CC BY Linden Lab. The rig parts are CC BY 3.0 Machinimatrix.org.
  - Texts: `licenses/AGPL-3.0-or-later.txt` and `licenses/CC-BY-3.0.txt`. The full notice and the list of authors are in `LICENSE.md` in each folder.
  - The `.pmm` files are converted and changed copies. The list of changes is in `README.md` in each folder.
  - **Source offer.** The source of these two files is the original Ruth2 and Roth2 files named, with their checksums, in `README.md` in each folder, together with the converter scripts `tools/make_ruth2_mesh.py` and `tools/make_roth2_mesh.py`. All of it is in the public project repository, https://github.com/pingledingle-tech/PinglyMotion, and at the tag of the release you have. If you cannot reach it, send a private message on GitHub to the project and ask for a copy.
- "Maxine" from the Max Avatar Project (`max/maxine.pmm`), by Ada Radius and the RuthAndRoth community. Licence: CC BY 4.0. Text: `licenses/CC-BY-4.0.txt`. Notice and changes: `LICENSE.txt` and `README.md` in that folder.

### OpenCollar example scripts

- In the folders `examples/improve/OpenCollar/` and `examples/improve/OpenCollar-more/`: scripts of the OpenCollar project and its contributors, copied unchanged, as plain text.
- The scripts are under the GNU General Public License version 2 with the "License Terms for OpenCollar". A verbatim copy of those terms is in the file `LICENSE` in each of the two folders and in `licenses/OpenCollar-License-Terms.txt`. The terms say that the scripts stay under that licence and stay full permission, and that changes must be open, with a change log. Each script keeps its own header.
- Only scripts are taken. Pictures, models, sounds, animations and notecards of OpenCollar, which are under other terms, are not used.
- Source: https://github.com/OpenCollarTeam/OpenCollar.

### Motion data

- Test motions in `examples/cmu/` and clips in `library/` from the CMU Graphics Lab Motion Capture Database, converted to BVH by cgspeed.com. The data used in this project was obtained from mocap.cs.cmu.edu. The database was created with funding from NSF EIA-0196217. CMU places no restrictions on its use and gives no licence text.
- Test motions in `examples/mesh2motion/` and clips in `library/` from the Mesh2Motion project. Licence: CC0 1.0 (no credit needed; given anyway).
- Each clip of the library carries its own licence and credit line in `library/index.json`.

### Not in the packages

- Luau and SLua are not included. The Lua trial build is off by default and is not part of any package.
- Aurora Viewer gave design ideas only. No Aurora code or files are in the packages.
- The MCP extension (`PinglyMotion.mcpb`) holds only PinglyMotion's own code and files. It needs Node, which Claude Desktop brings.
