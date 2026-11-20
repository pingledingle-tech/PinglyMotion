Branch: feat/selected-joint-card
Status: active
Pull request: #148
Files:
- src/jointcard.*
- src/jointturn.*
- src/CMakeLists.txt
- src/pinglymotionwindow.*
- CMakeLists.txt
- tests/jointcard/**
- docs/POSING_UX.md
- docs/ROADMAP.md
- docs/DECISIONS.md
- docs/PRODUCT.md

## What

Posing UX step 4 (#88): the Selected joint panel becomes a joint card for the
avatar: friendly name and SL name, IK switch, lock, reset, Rotate / Move tabs,
trackball plus X / Y / Z sliders with value fields, the allowed range on each
slider, a warning near a joint limit, and "Turn around: Joint / Avatar /
Screen". The prop page stays as it is. The whole-pose buttons (step 5), the
Hands tab (step 6), the pose library (step 11) and Settings (step 12) are not
part of this job.
