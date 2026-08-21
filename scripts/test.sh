#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
actual=$(mktemp -t k16-status-actual)
trap 'rm -f "$actual"' EXIT HUP INT TERM

plutil -lint "$repo_root/app/Info.plist" >/dev/null
plutil -lint "$repo_root/app/launchagent.plist" >/dev/null
plutil -convert xml1 -o /dev/null -- "$repo_root/config/k16-codex-controller.json"
node --check "$repo_root/tools/vendor-layout-proxy.mjs" 2>/dev/null || {
  echo "Note: Node.js is unavailable; skipped the optional proxy syntax check."
}

"$repo_root/scripts/build.sh"
"$repo_root/build/K16 Codex Lights.app/Contents/MacOS/K16CodexLights" \
  --replay "$repo_root/tests/status-fixture.jsonl" > "$actual"
diff -u "$repo_root/tests/status-expected.txt" "$actual"

aggregate_status=$(
  "$repo_root/build/K16 Codex Lights.app/Contents/MacOS/K16CodexLights" \
    --aggregate-replay \
    "$repo_root/tests/aggregate-thinking.jsonl" \
    "$repo_root/tests/aggregate-waiting.jsonl"
)
if [ "$aggregate_status" != "needs_input" ]; then
  echo "Expected cross-task aggregate status needs_input; received $aggregate_status" >&2
  exit 1
fi

echo "All tests passed."
