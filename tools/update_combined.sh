#!/usr/bin/env bash
# Rebuild the branch test/combined: current main plus the given pull requests.
# Usage: tools/update_combined.sh 120 121 122 [--push] [--dry-run]
# The branch is a test branch only and is never merged into main.
#
# Every pull request is checked first (issue 245): it must be open, come from a branch of this
# repository (not a fork) and be opened by the owner account. The exact head commit that was
# checked is merged, never whatever the pull request points to a moment later.
# --dry-run only checks the pull requests and prints what would be merged; it changes nothing.
# PINGLYMOTION_GH overrides the gh command (the tests use a mock).
set -u
repo="pingledingle-tech/PinglyMotion"
owner="pingledingle-tech"
gh_cmd="${PINGLYMOTION_GH:-gh}"
cd "$(git rev-parse --show-toplevel)" || exit 1
push=0; dry=0; prs=()
for a in "$@"; do
  case "$a" in
    --push) push=1 ;;
    --dry-run) dry=1 ;;
    *) prs+=("$a") ;;
  esac
done
[ "${#prs[@]}" -gt 0 ] || { echo "Usage: $0 <pr number>... [--push] [--dry-run]"; exit 2; }

# check every pull request before touching git
shas=()
for n in "${prs[@]}"; do
  case "$n" in
    ''|*[!0-9]*) echo "Not a pull request number: $n"; exit 2 ;;
  esac
  json="$("$gh_cmd" api "repos/$repo/pulls/$n")" || { echo "Cannot read pull request $n."; exit 1; }
  info="$(printf '%s' "$json" | python3 -c '
import json, sys
p = json.load(sys.stdin)
head = p.get("head") or {}
fields = [p.get("state"), (head.get("repo") or {}).get("full_name"), (p.get("user") or {}).get("login"), head.get("sha")]
print("\t".join(str(f or "-").replace("\t", " ") for f in fields))
')" || { echo "Cannot read pull request $n."; exit 1; }
  IFS=$'\t' read -r state head_repo login sha <<< "$info"
  if [ "$state" != "open" ]; then echo "REFUSED pull request $n: it is not open ($state)."; exit 1; fi
  if [ "$head_repo" != "$repo" ]; then echo "REFUSED pull request $n: it does not come from $repo (from: $head_repo)."; exit 1; fi
  if [ "$login" != "$owner" ]; then echo "REFUSED pull request $n: it was not opened by the owner account (opened by: $login)."; exit 1; fi
  case "$sha" in
    *[!0-9a-f]*|'') echo "REFUSED pull request $n: no valid head commit."; exit 1 ;;
  esac
  [ "${#sha}" -eq 40 ] || { echo "REFUSED pull request $n: no valid head commit."; exit 1; }
  shas+=("$sha")
  echo "checked pull request $n: head $sha"
done

if [ "$dry" = 1 ]; then
  for i in "${!prs[@]}"; do echo "would merge pull request ${prs[$i]} at ${shas[$i]}"; done
  exit 0
fi

git diff --quiet && git diff --cached --quiet || { echo "Working tree is not clean."; exit 1; }
git fetch -q origin main || exit 1
git checkout -q -B test/combined origin/main || exit 1
for i in "${!prs[@]}"; do
  n="${prs[$i]}"; sha="${shas[$i]}"
  git fetch -q origin "refs/pull/$n/head" || { echo "Cannot fetch pull request $n."; exit 1; }
  # merge the commit that was checked; stop if the pull request moved in between
  if [ "$(git rev-parse FETCH_HEAD)" != "$sha" ]; then
    echo "STOPPED: pull request $n changed after it was checked. Run the script again."
    exit 1
  fi
  if ! git merge -q --no-edit -m "Combined test build: merge pull request $n" "$sha"; then
    echo "CONFLICT merging pull request $n into test/combined."
    echo "Resolve it (keep each pull request's own changes), run: git commit, then merge the remaining pull requests by hand."
    exit 1
  fi
  echo "merged pull request $n at $sha"
done
if [ "$push" = 1 ]; then
  # --force-with-lease, never -f (CLAUDE.md). The branch was just rebuilt from main, so the local
  # test/combined has nothing in common with the remote one. Lease against the remote commit that
  # we fetch right now: if someone pushed in between, the push is refused.
  git fetch -q origin test/combined 2>/dev/null || true
  lease="$(git rev-parse --verify -q refs/remotes/origin/test/combined || true)"
  if [ -n "$lease" ]; then
    git -c push.negotiate=true push --force-with-lease="test/combined:$lease" origin test/combined || exit 1
  else
    # the remote branch does not exist yet: an empty expected value means "must not exist"
    git -c push.negotiate=true push --force-with-lease="test/combined:" origin test/combined || exit 1
  fi
  echo "pushed. Start the build: gh api -X POST repos/$repo/actions/workflows/build.yml/dispatches -f ref=test/combined"
fi
