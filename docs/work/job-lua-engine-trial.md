Branch: job/lua-engine-trial
Status: active
Pull request: pending
Files:
- CMakeLists.txt
- cmake/slua.cmake
- tests/luaengine/**
- .github/workflows/lua-engine.yml
- .github/workflows/build.yml
- CREDITS.md
- docs/DEV.md
- docs/ROADMAP.md
- docs/DECISIONS.md

## What

Trial build of Linden Lab's SLua engine (a Luau fork, MIT), fetched at a pinned commit behind the CMake option `PINGLYMOTION_LUA` (off by default), with a small test: a Lua state runs `return 1+1` and the sandbox has no file or system access. Nothing in the program uses it. Queue 2 in the jobs list (issue 74).
