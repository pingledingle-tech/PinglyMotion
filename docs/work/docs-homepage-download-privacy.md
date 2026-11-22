Branch: docs/homepage-download-privacy
Status: done
Pull request: #342
Files:
- docs/index.html
- docs/download.html
- docs/code-signing.html
- docs/privacy.html
- docs/site.css
- docs/site.js
- docs/README.md
- .github/ISSUE_TEMPLATE/**
- .github/workflows/pages.yml
- tools/site/build_status.py
- tests/test_build_status.py
- README.md
- docs/work/docs-homepage-download-privacy.md

## What

Homepage additions in all five languages: a download page (early preview, Windows zip and Linux AppImage from the GitHub releases), a code signing policy page, a privacy page, links to report a bug and to send a suggestion, and two issue templates ("Bug report", "Suggestion / feedback"). The shared style and script move into `docs/site.css` and `docs/site.js` so all pages use the same language picker and light/dark switch. The README download section links to the new pages.
