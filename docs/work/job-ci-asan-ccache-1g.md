Branch: job/ci-asan-ccache-1g
Status: done
Pull request: #411
Files:
- .github/workflows/build.yml

## What

The ASan/UBSan job fills its 300 MB compiler cache to 99-100 percent and runs 167-200 cleanups in every build (hit rate 45-56 percent, measured in four recent pull request runs). This job raises only `CCACHE_MAXSIZE` in the `asan-ubsan` job from `300M` to `1G`. Tests and security checks stay as they are. The result is compared with the measured baseline in a report comment.

Run 1 with 1G (pull request run 45141672387): hits 246/415 (59.3 %), no cleanups, cache 0.4 of 1.0 GB, job 15 min 7 s. A second run follows to see the steady state.
