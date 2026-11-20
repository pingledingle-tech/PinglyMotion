Branch: docs/homepage-status-section
Status: active
Pull request: pending
Files:
- docs/index.html
- .github/workflows/pages.yml
- tools/site/**
- tests/test_build_status.py

## What

A status section on the project homepage (issue 147): two or three GitHub
milestones with their progress in plain words, what waits for the owner's
test and what is being built. A GitHub Action builds it from GitHub's own
data (milestones, issues, open pull requests, the owner test list) when it
deploys the page to GitHub Pages, so nothing is kept up to date by hand.
