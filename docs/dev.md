# Development

## Nix + direnv

1. Install direnv and allow the shell.

   ```sh
   direnv allow
   ```

2. Create a local virtual environment for the Pebble tool.

   ```sh
   python3 -m venv .venv
   . .venv/bin/activate
   pip install --upgrade pip
   pip install pebble-sdk
   ```

3. Install the latest Rebble SDK into the repo-local SDK directory.

   ```sh
   export PEBBLE_SDK_HOME="$PWD/.pebble-sdk"
   pebble sdk install
   ```

## Build

```sh
scripts/build.sh
```

## Run in emulator

```sh
scripts/run-emulator.sh
```
