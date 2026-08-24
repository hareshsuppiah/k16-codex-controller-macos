#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_dir="$repo_root/build"
app="$build_dir/K16 Codex Lights.app"

if ! command -v clang >/dev/null 2>&1; then
  echo "clang is required. Install Xcode Command Line Tools with: xcode-select --install" >&2
  exit 1
fi

mkdir -p "$app/Contents/MacOS"
cp "$repo_root/app/Info.plist" "$app/Contents/Info.plist"

clang \
  -std=c11 \
  -Wall \
  -Wextra \
  -O2 \
  -framework CoreFoundation \
  -framework IOKit \
  "$repo_root/src/k16codexlights.c" \
  -o "$app/Contents/MacOS/K16CodexLights"

codesign --force --deep --sign - "$app"
codesign --verify --deep --strict "$app"

echo "Built: $app"
