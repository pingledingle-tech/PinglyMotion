Branch: ci/pages-deploy-retry
Status: done
Pull request: #283
Files:
- .github/workflows/pages.yml

## What

The homepage deploy after the five-language page was merged failed once with
a transient GitHub error (deploy-pages found no "github-pages" artifact right
after it was uploaded), so the live site kept the old page until a re-run.
The workflow now retries the deploy once after a short wait.
