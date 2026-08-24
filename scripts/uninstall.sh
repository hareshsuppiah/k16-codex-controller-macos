#!/bin/sh
set -eu

label="io.github.hareshsuppiah.k16codexlights"
domain="gui/$(id -u)"
installed_app="$HOME/Applications/K16 Codex Lights.app"
launch_agent="$HOME/Library/LaunchAgents/$label.plist"
karabiner_asset="$HOME/.config/karabiner/assets/complex_modifications/k16-codex-controller.json"

if [ "${1:-}" != "--yes" ]; then
  printf 'Remove the K16 app, login service, and supplied Karabiner rule file? [y/N] '
  read -r answer
  case "$answer" in
    y|Y|yes|YES) ;;
    *) echo "Cancelled."; exit 0 ;;
  esac
fi

launchctl bootout "$domain/$label" >/dev/null 2>&1 || true
rm -f "$launch_agent" "$karabiner_asset"
rm -rf "$installed_app"

echo "Removed K16 Codex Controller."
echo "You can remove its Input Monitoring entry manually in System Settings."
