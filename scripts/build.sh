#!/usr/bin/env bash
# Build helper for the Pebble watchface, keeping tool discovery consistent.
set -euo pipefail

if [ -n "${XDG_BIN_HOME:-}" ]; then
  export PATH="$XDG_BIN_HOME:$PATH"
else
  export PATH="$HOME/.local/bin:$PATH"
fi

export PATH="/usr/bin:/bin:$PATH"

use_nix_gcc=false
gcc_version="$(gcc --version 2>/dev/null || true)"
if [ -z "$gcc_version" ]; then
  use_nix_gcc=true
else
  shopt -s nocasematch
  case "$gcc_version" in
    *clang*) use_nix_gcc=true ;;
  esac
  shopt -u nocasematch
fi

if [ "$use_nix_gcc" = true ]; then
  gcc_bin_dir="$(nix-shell -p gcc --run 'dirname "$(command -v gcc)"')"
  export PATH="$gcc_bin_dir:$PATH"
fi

unset CC
unset CXX

if command -v pebble >/dev/null 2>&1; then
  pebble_cmd="$(command -v pebble)"
  "$pebble_cmd" build
elif command -v uv >/dev/null 2>&1; then
  uv tool run pebble build
else
  echo "pebble-tool not found; install with: uv tool install pebble-tool --python 3.13" >&2
  exit 1
fi
