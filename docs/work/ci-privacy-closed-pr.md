Branch: ci/privacy-closed-pr
Status: done
Pull request: #73
Files:
- .github/workflows/privacy.yml

## What

Editing the description of a pull request that is already merged starts the
`privacy` workflow. Its commit step then fails (exit 1 from git), because the
branch was deleted at the merge and its commits cannot be fetched. The text
step still runs and passes. Skip the commit step for closed pull requests; the
title and description are still checked.
