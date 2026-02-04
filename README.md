# Pebble Vipassana

Pebble Vipassana is a Pebble Watchface for servers and students attending Vipassana Courses as taught by S.N. Goenka.

The purpose of this watchface is to be able to use the Pebble Watch during a course, in such a way as to aid in the establishment of the technique by removing unnecessary time, and focusing on what is now, and what will come next.

## Features

- Shows current day and days remaining in compact format (e.g., `0/11 5:42`)
- Shows battery percentage (large, clean display)
- Shows current session/activity (large, wraps across multiple lines)
- Shows next activity
- Minimal, distraction-free design
- Settings page on mobile phone (due to Pebble OS limitation)
  - Course start date (arrival day)
  - Service period start date
  - Course type (10 day, Satipatthana, 20 day, etc.)
  - Course role (Student or Server)
  - Room number
  - Pagoda cell number
  - Dining hall number
  - Dhamma Hall cushion number

## Display Layout

```
0/11 5:42          ← Day/Days Left/Time to Next
85%                ← Battery percentage
Meditation in      ← Current activity (wraps)
Room
Breakfast          ← Next activity
```

## Installation & Setup

### Prerequisites
- Pebble watch with Rebble services configured
- Pebble app installed on phone (iOS/Android)
- Pebble CLI tools installed (for development)

### Installing to Watch

1. **Enable Developer Mode**
   - Open the Pebble mobile app
   - Go to **Settings** → **Devices** → Select your watch
   - Enable **"Developer Connection"**

2. **Login to Pebble**
   ```sh
   pebble login
   ```

3. **Install to Watch**
   ```sh
   pebble install --cloudpebble
   ```
   
   Or install the `.pbw` file directly:
   ```sh
   pebble install build/pebble-vipassana.pbw
   ```

### Configuring Settings

1. Open the Pebble app on your phone
2. Go to **My Pebbles** → Select your watch → **Watchfaces**
3. Find **Pebble Vipassana** and tap the **Settings** button
4. Configure:
   - **Course Start Date**: First day of course (arrival day at midnight)
   - **Service Start Date**: First day of service period (if serving)
   - **Course Type**: 10-day, 20-day, etc.
   - **Course Role**: Student or Server
   - **Location Numbers**: Room, Pagoda Cell, Dining Hall, Cushion

**Important**: Set dates to midnight (00:00) in your local timezone. The app uses wall-clock time and ignores timezones.

## Building from Source

See [docs/dev.md](docs/dev.md) for development setup and build instructions.

## Technical Notes

### Timezone Handling
The app uses **local calendar dates** exclusively. All times are "wall clock time":
- When you set "Feb 4, 2026 00:00" as course start, it means Feb 4 at midnight in whatever timezone your watch is currently set to
- Schedule times (e.g., 14:30 meditation) are interpreted as local time
- Day boundaries occur at midnight local time
- This ensures the app works correctly regardless of timezone or DST changes

### Text Rendering
- Buffer sizes are carefully tuned to prevent truncation
- Activity names wrap across multiple lines when needed
- Top line uses compact time format (no leading zeros: `5:42` not `05:42`)

### Schedule Data
Course schedules are hardcoded in `src/c/schedule.c` with separate timetables for:
- Students vs. Servers
- Different day types (Arrival, Anapana, Transition, Vipassana, Metta, Departure)
- Different course types (10-day, 20-day, Satipatthana)

## See Also

- [gotling/vipassana-pebble](https://github.com/gotling/vipassana-pebble) - Original inspiration
- [Rebble SDK Documentation](https://developer.rebble.io/sdk/)
- [Architecture Documentation](docs/architecture.md)
- [Development Guide](docs/dev.md)
