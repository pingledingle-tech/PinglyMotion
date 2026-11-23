Branch: job/ci-asan-ccache-1g
Status: active
Pull request: #411
Files:
- .github/workflows/build.yml

## What

The ASan/UBSan job fills its 300 MB compiler cache to 99-100 percent and runs 167-200 cleanups in every build (hit rate 45-56 percent, measured in four recent pull request runs). This job raises only `CCACHE_MAXSIZE` in the `asan-ubsan` job from `300M` to `1G`. Tests and security checks stay as they are. The result is compared with the measured baseline in a report comment.
