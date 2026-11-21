Branch: ci/pin-workflow-tools
Status: done
Pull request: #258
Files:
- .github/workflows/reference-fetch.yml
- .github/workflows/blender.yml
- tools/blender/ci-requirements.txt

## What

Issue 246: the reference-fetch and Blender workflows pin their actions by full commit, check out without keeping credentials, and pin uv (with hashes) and the bpy module to exact versions.
