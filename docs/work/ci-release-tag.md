Branch: ci/release-tag
Status: done
Pull request: pending
Files:
- .github/workflows/release.yml

## What

The ruleset "Protect release tags" does not let the Actions token create a `v*` tag, so publishing a release from a manual run would fail after 40 minutes of builds. The release workflow now never creates the tag: a person pushes it, and a manual run with publish on checks at the start that the tag exists and points at the built commit. The release is made with `--verify-tag`. Not run end to end: a dry run (publish off) takes about 40 minutes and is left for the next release.
