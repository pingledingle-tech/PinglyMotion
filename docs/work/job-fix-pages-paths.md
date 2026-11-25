Branch: job/fix-pages-paths
Status: done
Pull request: #515
Files:
- tools/ci/auto_merge.py

## What

Bring PAGES_PATHS in the auto-merge tool back in step with the push paths of the Pages workflow (the manual paths were added there but not here), so the test that compares them passes again.
