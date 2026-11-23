Branch: job/ai-connection
Status: active
Pull request: #413
Files:
- src/controlindicator.*
- src/controlserver.*
- src/aiassistant.*
- src/aiconnectdialog.*
- src/pinglymotionwindow.*
- src/usersettings.*
- src/CMakeLists.txt
- tests/CMakeLists.txt
- tests/aiassistant/**
- tests/tst_control.cpp
- tests/dialogfit/**
- tests/statusbar/**
- tests/test_mcpb.py
- tests/test_mcp_node.py
- tools/mcp/**
- tools/ci/package_mcp.sh
- .github/workflows/build.yml
- .github/workflows/release.yml
- translations/pinglymotion_en.ts
- README.md
- docs/mockup/index.html
- docs/CONTROL.md
- docs/PRODUCT.md
- docs/UX_DECISIONS.md
- docs/DEV.md
- docs/TEACHING_TEXTS.md

## What

AI connection (issue 7): one "AI Assistant" button in the status bar (off at every start), a guided setup
("Which AI do you use?" Claude, ChatGPT, Other), a "Connect ChatGPT" button that adds PinglyMotion to the
ChatGPT desktop app's `config.toml` (backup first, nothing else changed), the old "Remote control is on" box
and the Options menu entry removed, and an `mcp/` folder in the Windows zip with the extension, the Node
server and a README with all three guides and "How it's kept safe".
