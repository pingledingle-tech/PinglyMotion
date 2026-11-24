Branch: job/web-line-drawing
Status: done
Pull request: #484
Files:
- src/skinrenderer.cpp
- src/skinrenderer.h
- tests/linedraw/**
- CMakeLists.txt
- docs/DEV.md
- .github/workflows/build.yml

## What

Draw the lines of the 3D view (bones, axes, rings, grid) as triangles built on the CPU instead of with a geometry shader, so the same shaders work in WebGL 2 later. One function gives the shader header (GLSL 3.30 core on the desktop, GLSL ES 3.00 on the web). Looks the same as before.
