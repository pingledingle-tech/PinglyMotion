Branch: ci/dev-setup
Status: done
Pull request: pending
Files:
- CLAUDE.md
- docs/DEV.md
- tools/setup_clone.sh
- tools/dev_setup.sh
- .github/pull_request_template.md

## What

Every session starts the same way: `tools/setup_clone.sh` sets the commit author and installs a pre-push privacy check; `tools/dev_setup.sh` installs and configures the build; `docs/DEV.md` holds the build, test, sanitizer, picture and git/GitHub tricks; a pull request template asks for docs, issue link, job file and whether the user sees a change on screen. `CLAUDE.md` now says that technical jobs merge when the required checks are green and visible jobs wait for the owner's Windows test.
