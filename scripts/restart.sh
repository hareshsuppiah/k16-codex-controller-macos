#!/bin/sh
set -eu

label="io.github.hareshsuppiah.k16codexlights"
domain="gui/$(id -u)"
launch_agent="$HOME/Library/LaunchAgents/$label.plist"

if [ ! -f "$launch_agent" ]; then
  echo "Not installed. Run ./scripts/install.sh first." >&2
  exit 1
fi

launchctl bootout "$domain/$label" >/dev/null 2>&1 || true
launchctl bootstrap "$domain" "$launch_agent"
launchctl print "$domain/$label" | grep -E 'state =|program =|last exit code' || true
