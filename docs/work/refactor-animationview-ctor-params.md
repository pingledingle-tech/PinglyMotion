Branch: refactor/animationview-ctor-params
Status: done
Pull request: #279
Files:
- src/animationview.*
- docs/CODE_AUDIT.md
- docs/work/refactor-animationview-ctor-params.md

## What

Remove the unused `Animation*` parameter of the `AnimationView` constructor (nobody passes it; the window gives the scene its animations). Part of issue 200. Nothing changes on screen.
