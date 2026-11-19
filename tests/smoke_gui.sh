#!/bin/bash
# Start PinglyMotion with each test file and check that it is still running
# after a few seconds (no crash). Needs a display: run with xvfb-run.
# Usage: tests/smoke_gui.sh path/to/pinglymotion
set -u
APP="$(realpath "$1")"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT/bin"   # the program looks for ./data

FAILED=0
for FILE in "" "$ROOT/examples/jump.avm" "$ROOT/examples/throne.avm" \
            "$ROOT/bin/data/Relaxed.bvh" "$ROOT/tools/legacy/SLFemale.bvh"; do
  NAME="${FILE:-new animation}"
  timeout 8 "$APP" $FILE > /tmp/smoke.log 2>&1
  CODE=$?
  if [ $CODE -eq 124 ]; then
    echo "OK    $NAME"
  else
    echo "FAIL  $NAME (exit code $CODE)"
    tail -20 /tmp/smoke.log
    FAILED=1
  fi
done
exit $FAILED
