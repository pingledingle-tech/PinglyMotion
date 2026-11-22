Branch: cleanup/scene-ownership
Status: done
Pull request: #242
Files:
- src/animationview.*
- src/scene.*
- src/pinglymotion.*
- src/meshmodel.*
- src/pinglymotionwindow.cpp

## What

First step of the scene clean-up (code audit task C9): the window creates and owns the scene and gives it to the 3D view; the 'temporary' scaffolding and its stale comments (GLUT) are removed or explained. No change on screen. Part of the audit task; the rest stays open.
