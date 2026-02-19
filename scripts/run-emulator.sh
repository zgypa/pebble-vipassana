#!/usr/bin/env bash
# Install the watchface on the Pebble emulator.
set -euo pipefail

if [ -n "${XDG_BIN_HOME:-}" ]; then
  export PATH="$XDG_BIN_HOME:$PATH"
else
  export PATH="$HOME/.local/bin:$PATH"
fi

if command -v pebble >/dev/null 2>&1; then
  pebble install --emulator basalt
elif command -v uv >/dev/null 2>&1; then
  uv tool run pebble install --emulator basalt
else
  echo "pebble-tool not found; install with: uv tool install pebble-tool --python 3.13" >&2
  exit 1
fi
