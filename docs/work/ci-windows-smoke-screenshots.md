Branch: ci/windows-smoke-screenshots
Status: done
Pull request: #142
Files:
- .github/workflows/build.yml
- tools/ci/smoke_screenshots.py
- docs/DEV.md

## What

The Windows build runs the packed program once with remote control on, opens two example
files from the zip and takes a screenshot of each (artifact `PinglyMotion-windows-smoke`),
so an AI session can look at a pull request's pictures before the project owner tests (issue #129).
