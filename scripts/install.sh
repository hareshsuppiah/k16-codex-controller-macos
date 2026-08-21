#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
label="io.github.hareshsuppiah.k16codexlights"
domain="gui/$(id -u)"
installed_app="$HOME/Applications/K16 Codex Lights.app"
program="$installed_app/Contents/MacOS/K16CodexLights"
launch_agent="$HOME/Library/LaunchAgents/$label.plist"
karabiner_asset="$HOME/.config/karabiner/assets/complex_modifications/k16-codex-controller.json"

if [ "$(uname -s)" != "Darwin" ]; then
  echo "This installer supports macOS only." >&2
  exit 1
fi

"$repo_root/scripts/build.sh"

mkdir -p \
  "$HOME/Applications" \
  "$HOME/Library/LaunchAgents" \
  "$HOME/.config/karabiner/assets/complex_modifications"

ditto "$repo_root/build/K16 Codex Lights.app" "$installed_app"
cp "$repo_root/config/k16-codex-controller.json" "$karabiner_asset"
cp "$repo_root/app/launchagent.plist" "$launch_agent"
plutil -replace ProgramArguments.0 -string "$program" "$launch_agent"
plutil -lint "$launch_agent" >/dev/null

launchctl bootout "$domain/$label" >/dev/null 2>&1 || true
launchctl bootstrap "$domain" "$launch_agent"

echo
echo "Installed K16 Codex Controller."
echo
echo "Manual steps:"
echo "1. Enable K16 Codex Lights in System Settings > Privacy & Security > Input Monitoring."
echo "2. Enable the K16 Codex Controller rules in Karabiner-Elements > Complex Modifications."
echo "3. Configure the three Codex shortcuts documented in README.md."
echo "4. Run ./scripts/restart.sh after granting Input Monitoring."

open "x-apple.systempreferences:com.apple.preference.security?Privacy_ListenEvent" >/dev/null 2>&1 || true
