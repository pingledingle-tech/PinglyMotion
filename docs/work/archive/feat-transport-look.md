Branch: feat/transport-look
Status: done
Pull request: #275
Files:
- src/transportbar.*
- src/transportlook.*
- src/theme.*
- src/lucideicons.*
- src/icons/lucide/LICENSE
- src/icons.qrc
- src/icons/play.png
- src/icons/play.xcf
- src/icons/stop.png
- src/CMakeLists.txt
- tests/transportbar/**
- docs/UX_DECISIONS.md
- docs/TEACHING_TEXTS.md
- CREDITS.md

## What

The transport buttons under the timeline in the design system
(docs/DESIGN_CHARTER.md, docs/DESIGN_SYSTEM.md): Start is the main button,
Stop a danger button, Replay from start a normal button, all with the same
shape, Lucide icons and colours from the theme tokens in both themes. A
button that cannot be clicked says why. The buttons, their texts and what
they do stay the same (the project owner approved them in test AR).
