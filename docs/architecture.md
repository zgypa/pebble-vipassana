# Architecture

This document explains the components and data flow for the watchface.

## Goals

- Provide a minimal, distraction-free watchface for Vipassana courses.
- Keep the display focused on the current day/session and next transition.
- Use simple wall-clock time to avoid timezone complexity.

## Modules

### 1. Course Composition Engine (`schedule.c`)
Maps course types to ordered day types and provides schedule lookups.

**Key Functions:**
- `schedule_get_day_type()` - Maps course day number to day type (ARRIVAL, ANAPANA, TRANSITION, VIPASSANA, METTA, DEPARTURE)
- `schedule_get_day()` - Returns the activity schedule for a given course type, role, and day
- `schedule_current_index()` - Finds the current activity based on minutes since midnight

### 2. Schedule Engine (`schedule.c`)
Stores day-type timetables with activity times in minutes since midnight.

**Data Structures:**
- Activities stored as `{minutes, label, kind, meditation}` tuples
- Separate schedules for Students vs Servers
- Times are in minutes since midnight (e.g., `14 * 60 + 30` = 870 = 14:30)

### 3. Settings Sync (`settings.c` + `src/pkjs/index.js`)
Handles configuration persistence and phone ↔ watch communication.

**Storage:**
- Watch persists settings using Pebble's `persist_*` API
- Phone config page sends settings via AppMessage
- Watch sends current settings back to phone on startup (for "Sync from Watch" feature)

**Date/Time Handling:**
- Dates stored as `{year, month, day, hour, minute}` structs
- ISO format for phone communication: `"YYYY-MM-DD HH:MM"`
- `settings_datetime_to_time()` uses `mktime()` to convert to time_t

### 4. Display Composition (`pebble-vipassana.c`)
Renders the watchface UI with current and next activities.

**Display Layers:**
```
Y=0,  H=34  → Top line (Day/Left/Time)  - GOTHIC_28_BOLD
Y=34, H=24  → Battery percentage        - GOTHIC_24_BOLD  
Y=74, H=64  → Current activity (wraps)  - GOTHIC_28_BOLD
Y=138,H=20  → Next activity             - GOTHIC_18_BOLD
```

**Update Cycle:**
- Tick handler runs every minute
- Calculates current day relative to course start
- Looks up current and next activities from schedule
- Updates all text layers

## Data Flow

1. **Initialization:**
   - Watch loads settings from persistent storage
   - Opens AppMessage channel to phone
   - Sends current settings to phone (for UI sync)
   - Subscribes to minute ticks

2. **Tick Handler (Every Minute):**
   - Get current local time from system
   - Calculate course day using calendar date arithmetic
   - Determine mode (PRE_COURSE, SERVICE, COURSE, POST)
   - Look up current activity from schedule
   - Look up next activity
   - Update display layers

3. **Settings Update:**
   - User opens settings page on phone
   - JavaScript fetches current settings from watch
   - User modifies and saves
   - JavaScript sends new settings to watch via AppMessage
   - Watch persists settings and refreshes display

## Course Composition and Day Types

Each course is composed of day types. For example, on a regular 10-day course:

| Day | Type | Description |
|-----|------|-------------|
| 0 | ARRIVAL | Registration, orientation, course begins at 20:00 |
| 1-3 | ANAPANA | Learning breath awareness technique |
| 4 | TRANSITION | Teaching of Vipassana begins |
| 5-9 | VIPASSANA | Body scanning practice (same schedule as Anapana) |
| 10 | METTA | Loving-kindness practice, noble silence ends |
| 11 | DEPARTURE | Cleaning, checkout |

Longer courses extend these sequences. Each day type has separate schedules for Students and Servers.

## Timezone and Time Handling

### Design Decision: Wall-Clock Time Only

The app uses **local calendar dates** exclusively to avoid timezone complexity:

```c
// ✓ CORRECT: Compare calendar dates directly
int days_between_dates(int year1, int month1, int day1,
                       int year2, int month2, int day2);

// ✗ WRONG: Unix timestamp arithmetic affected by timezone
int days_between(time_t start, time_t end) {
  return (end - start) / 86400;  // Breaks across DST/timezone changes
}
```

### Why This Matters

Unix timestamps represent absolute moments in UTC. When you divide by 86400 to get "days", the boundary is at midnight UTC, not midnight local time.

**Example Problem (Old Code):**
- Course starts: Feb 4, 2026 00:00 America/Chicago (UTC-6)
- This is: Feb 4, 2026 06:00 UTC
- Current time: Feb 4, 2026 14:18 Chicago = Feb 4, 2026 20:18 UTC
- Days calculation: `(timestamp_current / 86400) - (timestamp_start / 86400)`
- Day boundary at 18:00 Chicago time (midnight UTC), not midnight local
- Result: Wrong day number before 18:00!

**Solution (Current Code):**
- Store dates as `{year, month, day}` tuples
- Compare calendar dates directly using `mktime()` at noon to avoid DST edge cases
- Schedule times are minutes since midnight (timezone-agnostic)
- Mode determination prioritizes course state over service state

### Buffer Sizes

Text buffers must be large enough for formatted output:

```c
static char s_time_buffer[16];  // "0/11 5:42" = 10 chars + null = 11 bytes minimum
static char s_battery_buffer[12]; // "100%" = 4 chars, but allow room
static char s_session_buffer[48]; // Activity names can be long
```

**Learned the Hard Way:** Originally `s_time_buffer[8]` truncated `"0/11 5:42"` to `"0/11 5:"`.

### Text Wrapping

Activity names can be long ("Meditation in Room"). The session layer uses:
- Height: 64px (enough for 2 lines of GOTHIC_28_BOLD)
- Overflow mode: `GTextOverflowModeTrailingEllipsis`
- Center alignment

In practice, GOTHIC_28_BOLD doesn't wrap well. Text gets truncated with "..." if too long. Future improvement: Use a smaller font or custom text splitting.

### Mode Determination Priority

The app checks states in this order:

1. **Are we in the course period?** (course_start ≤ today ≤ course_start + 11 days)
   - Yes → MODE_COURSE
   - No → Continue

2. **Is course over?**
   - Yes → MODE_POST
   - No → Continue

3. **Are we in service period?** (service_start ≤ today < course_start)
   - Yes → MODE_SERVICE
   - No → Continue

4. **Both are in the future**
   - Show whichever comes first (MODE_PRE_SERVICE or MODE_PRE_COURSE)

**Why this order matters:** If both course_start and service_start are set to the same date (common case), we want MODE_COURSE to take priority.
