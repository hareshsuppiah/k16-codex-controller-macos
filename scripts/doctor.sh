#!/bin/sh
set -eu

label="io.github.hareshsuppiah.k16codexlights"
domain="gui/$(id -u)"
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
check "installed lighting app" test -x "$HOME/Applications/K16 Codex Lights.app/Contents/MacOS/K16CodexLights"
check "installed Karabiner rules" test -f "$HOME/.config/karabiner/assets/complex_modifications/k16-codex-controller.json"
check "login service" launchctl print "$domain/$label"

usb_report=$(system_profiler SPUSBDataType 2>/dev/null || true)
if printf '%s\n' "$usb_report" | grep -qi '0x36ae' && printf '%s\n' "$usb_report" | grep -qi '0x2475'; then
  echo "OK   tested K16 USB identity (36ae:2475)"
else
  echo "MISS tested K16 USB identity (36ae:2475)"
  failures=$((failures + 1))
fi

echo
echo "Input Monitoring cannot be verified reliably from this script."
echo "Check System Settings > Privacy & Security > Input Monitoring manually."

exit "$failures"
