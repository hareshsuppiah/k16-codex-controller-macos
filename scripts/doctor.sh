#!/bin/sh
set -eu

label="io.github.hareshsuppiah.k16codexlights"
domain="gui/$(id -u)"
program="$HOME/Applications/K16 Codex Lights.app/Contents/MacOS/K16CodexLights"
launch_agent="$HOME/Library/LaunchAgents/$label.plist"
failures=0

check() {
  description=$1
  shift
  if "$@" >/dev/null 2>&1; then
    printf 'OK   %s\n' "$description"
  else
    printf 'MISS %s\n' "$description"
    failures=$((failures + 1))
  fi
}

check "macOS" test "$(uname -s)" = "Darwin"
check "clang" command -v clang
check "Karabiner-Elements" test -d "/Applications/Karabiner-Elements.app"
check "installed lighting app" test -x "$program"
check "installed Karabiner rules" test -f "$HOME/.config/karabiner/assets/complex_modifications/k16-codex-controller.json"
check "installed LaunchAgent" test -f "$launch_agent"
check "login service" launchctl print "$domain/$label"

if [ -x "$program" ]; then
  derived_status=$("$program" --status-once 2>/dev/null || true)
  case "$derived_status" in
    idle|thinking|complete|needs_input|error)
      printf 'OK   aggregate Codex status (%s)\n' "$derived_status"
      ;;
    *)
      echo "MISS aggregate Codex status"
      failures=$((failures + 1))
      ;;
  esac
fi

hid_report=$(ioreg -r -c IOHIDDevice -l 2>/dev/null || true)
if printf '%s\n' "$hid_report" | grep -q '"VendorID" = 13998' && \
   printf '%s\n' "$hid_report" | grep -q '"ProductID" = 9333'; then
  echo "OK   tested K16 USB identity (36ae:2475)"
else
  echo "MISS tested K16 USB identity (36ae:2475)"
  failures=$((failures + 1))
fi

echo
echo "Input Monitoring cannot be verified reliably from this script."
echo "Check System Settings > Privacy & Security > Input Monitoring manually."

exit "$failures"
