Branch: ci/codeql-apt-cache
Status: done
Pull request: #44
Files:
- .github/workflows/codeql.yml

## What

Cache the downloaded apt packages (the Qt development packages) between runs
of the CodeQL C++ job, the same way build.yml does, so the analysis starts
sooner. The check name `analyze` does not change.
