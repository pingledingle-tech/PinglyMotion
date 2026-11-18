Branch: ci/codeql-apt-cache
Status: active
Pull request: pending
Files:
- .github/workflows/codeql.yml

## What

Cache the downloaded apt packages (the Qt development packages) between runs
of the CodeQL C++ job, the same way build.yml does, so the analysis starts
sooner. The check name `analyze` does not change.
