# Development

This guide describes the local SDK setup, build workflow, and deployment for the repo.

## Prerequisites

- macOS or Linux (Windows via WSL)
- Nix with flakes enabled (optional but recommended)
- direnv (optional, for auto-environment)
- Python 3.11+ (for pebble-tool)

## Setup

### Nix + direnv

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

### Manual Setup (without Nix)

1. Install Python 3.11+

2. Install pebble-tool:
   ```sh
   pip install pebble-tool
   ```

3. Install the Pebble SDK:
   ```sh
   export PEBBLE_SDK_HOME="$HOME/.pebble-sdk"
   pebble sdk install latest
   ```

## Build

Build the watchface for all platforms:

```sh
./scripts/build.sh
```

This generates `build/pebble-vipassana.pbw`.

**Note:** The build script automatically runs `scripts/fix-capabilities.py` to ensure the `capabilities` array in `appinfo.json` includes `"configurable"` (required for settings button to appear on modern Rebble/Pebble Core apps).

## Run in Emulator

```sh
./scripts/run-emulator.sh
```

Or manually:
```sh
pebble build
pebble install --emulator basalt
```

## Deploy to Watch

### First Time Setup

1. **Enable Developer Mode**
   - Open the Pebble mobile app
   - Go to **Settings** → **Devices** → Select your watch
   - Enable **"Developer Connection"**

2. **Login to Pebble**
   ```sh
   pebble login
   ```

### Install to Watch

Via CloudPebble (requires Developer Connection enabled):
```sh
pebble install --cloudpebble
```

Or directly via USB/Bluetooth:
```sh
pebble install --phone <PHONE_IP>
```

Or install the PBW file:
```sh
pebble install build/pebble-vipassana.pbw
```

## Viewing Logs

To debug the watchface, view real-time logs from the watch:

```sh
pebble logs
```

Or with specific phone IP:
```sh
pebble logs --phone <PHONE_IP>
```

Or from emulator:
```sh
pebble logs --emulator basalt
```

Look for debug messages prefixed with the source file:
```
[15:10:18] pebble-vipassana.c:281> Current date/time: 2026-02-04 15:10
[15:10:18] pebble-vipassana.c:324> Minutes until next: 18, Duration: '0:18'
```

## Project Structure

```
.
├── src/
│   ├── c/
│   │   ├── pebble-vipassana.c    # Main watchface logic
│   │   ├── schedule.c            # Schedule data and lookup
│   │   ├── settings.c            # Settings persistence
│   │   └── settings_ui.c         # On-watch settings (unused)
│   ├── pkjs/
│   │   └── index.js              # Phone settings UI (PebbleKit JS)
│   └── resources/                # Icons, fonts (if any)
├── scripts/
│   ├── build.sh                  # Main build script
│   ├── fix-capabilities.py       # Post-build capability patcher
│   └── run-emulator.sh           # Launch emulator
├── build/                        # Build output
│   ├── pebble-vipassana.pbw      # Final package
│   └── appinfo.json              # Generated metadata
├── appinfo.json                  # App metadata template
└── wscript                       # Waf build configuration
```

## Code Conventions

### C Code
- Use `snake_case` for functions and variables
- Prefix static functions with `static`
- Use `k` prefix for constants (e.g., `kTimeY`, `kBatteryH`)
- Use `s_` prefix for static module variables (e.g., `s_settings`)

### Buffer Sizes
Always ensure buffers are large enough for formatted output:

```c
char buffer[16];
snprintf(buffer, sizeof(buffer), "%d/%d %s", day, left, time);
```

**Common pitfall:** `snprintf` includes null terminator in count. For `"0/11 5:42"` (10 chars), you need 11 bytes minimum.

### Time Handling
- Use `struct tm` from `localtime()` for all time operations
- Never use raw Unix timestamps for day calculations
- Store times as minutes since midnight (0-1439)
- Compare dates using calendar arithmetic, not timestamp division

```c
// ✓ CORRECT
int days_between_dates(int y1, int m1, int d1, int y2, int m2, int d2);

// ✗ WRONG (breaks across timezones)
int days = (time2 - time1) / 86400;
```

## Debugging Tips

### Text Truncation Issues
If text appears cut off:
1. Check buffer size in `pebble-vipassana.c` (e.g., `s_time_buffer`)
2. Check layer height constants (e.g., `kTimeH`)
3. Check font size - larger fonts need taller layers
4. Add debug logging to see actual string content:
   ```c
   APP_LOG(APP_LOG_LEVEL_DEBUG, "Buffer: '%s'", buffer);
   ```

### Schedule Not Updating
1. Verify course start date is set correctly (midnight local time)
2. Check mode determination in logs: `"Mode determination: days_until_service=X, days_until_course=Y"`
3. Verify current activity index: `"Current index: 9, Activity: Meditation in Room at 870 min"`

### Settings Not Saving
1. Check message keys match in `appinfo.json` and `src/pkjs/index.js`
2. Keys must be integers 10000-10008 (not 0-8)
3. View browser console in phone app for JavaScript errors

## Known Issues & Quirks

### Text Wrapping
GOTHIC_28_BOLD doesn't wrap well on Pebble. Long text gets truncated with "..." even with `GTextOverflowModeWordWrap`. The current workaround is to use shorter activity names or accept the ellipsis.

### Settings Button Visibility
The settings button only appears if:
1. `"configurable": true` in `appinfo.json`
2. `"configurable"` present in `capabilities` array

The Pebble SDK generates the first but not the second. The `fix-capabilities.py` script patches this automatically during build.

### Date Input on iOS
iOS Pebble app date pickers may show time selectors. Ignore them - times are automatically set to midnight (`00:00`) by the JavaScript code.

### AppMessage Size Limits
The watchface opens AppMessage with 512 bytes inbox, 256 bytes outbox. If adding more settings, these may need to increase:

```c
app_message_open(512, 256);  // in pebble-vipassana.c
```

## Contributing

When adding new features:
1. Add debug logging for new calculations
2. Update buffer sizes if adding longer strings
3. Test timezone handling with dates around DST transitions
4. Update this documentation and README.md
5. Test on real hardware (emulator doesn't catch all layout issues)
