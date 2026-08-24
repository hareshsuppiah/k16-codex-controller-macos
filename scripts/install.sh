#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
label="io.github.hareshsuppiah.k16codexlights"
legacy_label="au.com.hareshsuppiah.k16codexlights"
domain="gui/$(id -u)"
installed_app="$HOME/Applications/K16 Codex Lights.app"
built_app="$repo_root/build/K16 Codex Lights.app"
program="$installed_app/Contents/MacOS/K16CodexLights"
launch_agent="$HOME/Library/LaunchAgents/$label.plist"
legacy_launch_agent="$HOME/Library/LaunchAgents/$legacy_label.plist"
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

launchctl bootout "$domain/$legacy_label" >/dev/null 2>&1 || true
launchctl bootout "$domain/$label" >/dev/null 2>&1 || true
# Earlier manual launches are not owned by the LaunchAgent and can otherwise
# keep sending stale colours after an upgrade.
pkill -x K16CodexLights >/dev/null 2>&1 || true
shutdown_attempt=0
while pgrep -x K16CodexLights >/dev/null 2>&1 && [ "$shutdown_attempt" -lt 30 ]; do
  sleep 0.1
  shutdown_attempt=$((shutdown_attempt + 1))
done
if pgrep -x K16CodexLights >/dev/null 2>&1; then
  echo "An older K16CodexLights process did not stop; installation was cancelled." >&2
  exit 1
fi

app_replaced=yes
if [ -x "$program" ] && \
   cmp -s "$built_app/Contents/MacOS/K16CodexLights" "$program" && \
   cmp -s "$built_app/Contents/Info.plist" "$installed_app/Contents/Info.plist"; then
  app_replaced=no
else
  ditto "$built_app" "$installed_app"
fi
cp "$repo_root/config/k16-codex-controller.json" "$karabiner_asset"
cp "$repo_root/app/launchagent.plist" "$launch_agent"
program_arguments=$(printf '["%s"]' "$program")
plutil -replace ProgramArguments -json "$program_arguments" "$launch_agent"
plutil -lint "$launch_agent" >/dev/null

launchctl bootstrap "$domain" "$launch_agent"
if ! launchctl print "$domain/$label" >/dev/null 2>&1; then
  echo "The K16 background service did not load." >&2
  exit 1
fi

if [ -f "$legacy_launch_agent" ]; then
  legacy_backup="$legacy_launch_agent.disabled.$(date +%s)"
  mv "$legacy_launch_agent" "$legacy_backup"
  echo "Disabled obsolete login service: $legacy_backup"
fi

echo
echo "Installed K16 Codex Controller."
echo
echo "Manual steps:"
echo "1. Enable K16 Codex Lights in System Settings > Privacy & Security > Input Monitoring."
echo "2. Enable the K16 Codex Controller rules in Karabiner-Elements > Complex Modifications."
echo "3. Configure the three Codex shortcuts documented in README.md."
echo "4. Run ./scripts/restart.sh after granting Input Monitoring."

if [ "$app_replaced" = yes ]; then
  echo
  echo "The lighting helper changed in this installation."
  echo "If macOS kept an older Input Monitoring identity, remove K16 Codex Lights"
  echo "from that list, add the installed app again, and switch it on."
fi

open "x-apple.systempreferences:com.apple.preference.security?Privacy_ListenEvent" >/dev/null 2>&1 || true
