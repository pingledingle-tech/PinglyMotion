#!/usr/bin/env bash
# One command after cloning, so every session starts the same way:
#   tools/setup_clone.sh
# - sets the commit author to the project's GitHub noreply identity
# - installs a pre-push hook that runs the privacy check on what is about to be pushed
# Run it from anywhere inside the clone.
set -euo pipefail

root="$(git rev-parse --show-toplevel)"
cd "$root"

git config user.name "pingledingle-tech"
git config user.email "351915610+pingledingle-tech@users.noreply.github.com"

hook="$root/.git/hooks/pre-push"
cat > "$hook" <<'HOOK'
#!/usr/bin/env bash
# installed by tools/setup_clone.sh: stop a push that holds personal data
root="$(git rev-parse --show-toplevel)"
zero="0000000000000000000000000000000000000000"
status=0
while read -r local_ref local_sha remote_ref remote_sha; do
  [ "$local_sha" = "$zero" ] && continue          # deleting a branch
  base="$remote_sha"
  if [ "$base" = "$zero" ]; then                  # a new branch: compare with main when we have it
    base="$(git merge-base origin/main "$local_sha" 2>/dev/null || true)"
  fi
  python3 "$root/tools/check_privacy.py" "$base" "$local_sha" || status=1
done
exit $status
HOOK
chmod +x "$hook"

echo "Author:  $(git config user.name) <$(git config user.email)>"
echo "Hook:    $hook"
