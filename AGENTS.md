# AGENTS.md

This repository contains a Pebble watchface (C) plus PebbleKit JS settings UI.
Use this guide for build commands and local coding conventions.

## Tooling and rules

- Cursor rules: none found in `.cursor/rules/` or `.cursorrules`.
- Copilot rules: none found in `.github/copilot-instructions.md`.
- No lint or test runner is configured in this repo.

## Build and run

Primary build scripts:

- Build all platforms: `./scripts/build.sh`
- Build via make: `make build` (same as `./scripts/build.sh`)
- Build settings-only UI: `make build-settings` or `VIPASSANA_SETTINGS_ONLY=1 ./scripts/build.sh`
- Run emulator: `./scripts/run-emulator.sh` or `make run-emulator`
- Build settings-only and run emulator: `make run-settings`

Pebble CLI equivalents (manual):

- Build: `pebble build`
- Install to emulator: `pebble install --emulator basalt`
- Install to watch: `pebble install --cloudpebble` or `pebble install --phone <PHONE_IP>`
- Watch logs: `pebble logs` or `pebble logs --emulator basalt`

Notes about the build:

- `scripts/build.sh` runs `scripts/fix-capabilities.py` after `pebble build` to
  ensure `appinfo.json` contains the `configurable` capability.
- `wscript` uses `VIPASSANA_SETTINGS_ONLY=1` to compile with `-DSETTINGS_ONLY=1`.

## Linting and tests

- Lint: not configured.
- Tests: not configured. There is no unit/integration test runner.

Single-test guidance (best available alternative):

- There is no way to run a single automated test. For targeted verification,
  rebuild and use the emulator with `pebble logs` while exercising the feature.
- Focused run: `./scripts/build.sh && ./scripts/run-emulator.sh` then monitor
  logs with `pebble logs --emulator basalt`.

## Code style

### C (watchface)

Formatting and layout:

- Indentation is 2 spaces; brace style is K&R.
- Keep includes ordered: system headers first, then local headers.
- Prefer `static` for internal functions and module-level variables.

Naming conventions:

- Functions and variables: `snake_case`.
- Static module variables: prefix with `s_` (e.g., `s_settings`).
- Constants: prefix with `k` (e.g., `kTimeY`).
- Enum values use uppercase words with underscores.

Types and data flow:

- Use `struct tm` from `localtime()` for time operations.
- Store schedule times as minutes since midnight (0-1439).
- Prefer calendar arithmetic for day math; avoid raw timestamp division.
- Settings are stored in `DateTimeParts` and persisted via `persist_*` APIs.

Error handling and safety:

- Check return values (e.g., `app_message_outbox_begin`, `persist_exists`).
- Buffer sizes must include the null terminator; use `snprintf` with `sizeof`.
- If a buffer may be empty, explicitly set `buffer[0] = '\0'`.

Logging:

- Use `APP_LOG` with clear prefixes (file or module name) for debugging.
- Prefer actionable logs (e.g., computed times, mode decisions).

### PebbleKit JS (phone config)

Formatting and layout:

- ES5-style JavaScript; `var` and `function` declarations are the norm.
- No bundler or module system; keep everything in `src/pkjs/index.js`.
- String-heavy HTML is built via concatenation; keep it readable and consistent.

Naming conventions:

- Use lowerCamelCase for functions and variables.
- Keep Pebble message keys in the `keys` object and numeric values 10000-10008.

Error handling and behavior:

- Guard against missing values from `localStorage` or config webview.
- When closing the config page, handle failures and show a status message.
- Prefer `parseInt(value, 10)` for numeric fields sent to the watch.

### Imports and dependencies

- C: `#include <...>` for SDK/system headers, then `#include "..."` for local.
- JS: no imports; keep a single-file workflow.
- Python: standard library only; keep scripts minimal and self-contained.

## Domain-specific rules

- Course scheduling data lives in `src/c/schedule.c`; keep times in minutes.
- The watchface uses (doesn't display it) wall-clock (local) time; do not introduce UTC math.
- Buffer sizes are tuned to UI text; increase when labels grow.
- Settings keys must align between `appinfo.json` and `src/pkjs/index.js`.
- **CRITICAL**: The watchface NEVER displays wall-clock calendar dates (year/month/day).
  Only show course-relative information: day numbers (0-11), time until next activity, current day
  and days remaining. The entire purpose of this app is to remove calendar distractions
  during Vipassana courses.

## Project layout

- `src/c/pebble-vipassana.c`: main watchface logic and rendering.
- `src/c/schedule.c`: timetable data and day-type lookup.
- `src/c/settings.c`: settings persistence and date parsing.
- `src/pkjs/index.js`: phone configuration UI and AppMessage bridge.
- `scripts/build.sh`: build helper (selects pebble-tool via PATH/uv).
- `scripts/run-emulator.sh`: installs on the Basalt emulator.
- `scripts/fix-capabilities.py`: patches appinfo and rebuilds the PBW.
- `wscript`: Pebble SDK build configuration.

## Settings sync notes

- AppMessage keys are defined by `message_keys.h` generated from `package.json`.
- The JS `keys` map must match the numeric values 10000-10008.
- AppMessage sizes are currently opened at 512 inbox / 256 outbox bytes.
- When adding new settings, update both watch and phone mappings together.

## Time and date handling details

- Use calendar date math with `mktime()` at noon to avoid DST edges.
- Treat course/service dates as local dates (midnight local time).
- Store and transmit dates as `YYYY-MM-DD HH:MM` strings in settings.

## Suggested workflow for changes

- Make focused edits in `src/c/*.c` or `src/pkjs/index.js`.
- Build with `./scripts/build.sh` and install to emulator to verify UI.
- Use `pebble logs` to validate schedule calculations and settings sync.

## Common gotchas

- Pebble SDK can omit `configurable` capability; rely on `fix-capabilities.py`.
- GOTHIC_28_BOLD does not wrap well; long labels may ellipsize unexpectedly.
- Dates are stored as local calendar dates; do not derive days by timestamps.
