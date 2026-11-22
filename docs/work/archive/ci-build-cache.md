Branch: ci/build-cache
Status: done
Pull request: #6
Files:
- .github/workflows/build.yml
- .github/workflows/sanitizers.yml
- .github/workflows/fuzz.yml

## What

Faster checks: ccache for the Linux build and the sanitizer build, Qt cache
for the Windows build, downloaded apt packages cached in the Linux, sanitizer
and fuzz jobs. Caches are separate per job and scoped to the branch; the
Windows compile and CodeQL stay uncached.
