# Development

## Nix + direnv

1. Install direnv and allow the shell.

   ```sh
   direnv allow
   ```

2. Install the Pebble CLI with `uv`.

   ```sh
   uv tool install pebble-tool --python 3.13
   ```

   If `pebble` is not on your PATH, ensure `~/.local/bin` (or `XDG_BIN_HOME`) is in your PATH.

3. Install the latest Rebble SDK into the repo-local SDK directory.

   ```sh
   export PEBBLE_SDK_HOME="$PWD/.pebble-sdk"
   pebble sdk install latest
   ```

## Build

```sh
scripts/build.sh
```

## Run in emulator

```sh
scripts/run-emulator.sh
```
