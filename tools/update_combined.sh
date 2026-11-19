#!/usr/bin/env bash
# Rebuild the branch test/combined: current main plus the given pull requests.
# Usage: tools/update_combined.sh 120 121 122 [--push]
# The branch is a test branch only and is never merged into main.
set -u
cd "$(git rev-parse --show-toplevel)" || exit 1
push=0; prs=()
for a in "$@"; do
  if [ "$a" = "--push" ]; then push=1; else prs+=("$a"); fi
done
[ "${#prs[@]}" -gt 0 ] || { echo "Usage: $0 <pr number>... [--push]"; exit 2; }
git diff --quiet && git diff --cached --quiet || { echo "Working tree is not clean."; exit 1; }
git fetch -q origin main || exit 1
git checkout -q -B test/combined origin/main || exit 1
for n in "${prs[@]}"; do
  git fetch -q origin "refs/pull/$n/head" || { echo "Cannot fetch pull request $n."; exit 1; }
  if ! git merge -q --no-edit -m "Combined test build: merge pull request $n" FETCH_HEAD; then
    echo "CONFLICT merging pull request $n into test/combined."
    echo "Resolve it (keep each pull request's own changes), run: git commit, then merge the remaining pull requests by hand."
    exit 1
  fi
  echo "merged pull request $n"
done
if [ "$push" = 1 ]; then
  git -c push.negotiate=true push -f origin test/combined || exit 1
  echo "pushed. Start the build: gh api -X POST repos/pingledingle-tech/PinglyMotion/actions/workflows/build.yml/dispatches -f ref=test/combined"
fi
