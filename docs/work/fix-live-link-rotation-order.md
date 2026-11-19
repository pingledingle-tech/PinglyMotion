Branch: fix/live-link-rotation-order
Status: done
Pull request: #77
Files:
- src/bvh.cpp
- tests/tst_auroralink.cpp
- tests/CMakeLists.txt

## What

Fixes issue #64: the live link `pose` message composed the rotation channels of a joint in the reverse order of the 3D view, the IK and the .anim export. A joint turned on two or three axes therefore looked different in Aurora. The link now multiplies each channel on the right, in file order, and a test compares the link quaternion with the one the .anim export writes. Visible change (what Aurora shows): the project owner tests it on Windows before merge.
