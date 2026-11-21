Branch: refactor/animationview-ctor-params
Status: active
Pull request: pending
Files:
- src/animationview.*
- docs/CODE_AUDIT.md
- docs/work/refactor-animationview-ctor-params.md

## What

Remove the unused `Animation*` parameter of the `AnimationView` constructor (nobody passes it; the window gives the scene its animations). Part of issue 200. Nothing changes on screen.
