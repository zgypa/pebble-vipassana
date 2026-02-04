# Architecture

This document explains the planned components and data flow for the watchface.

## Goals

- Provide a minimal, distraction-free watchface for Vipassana courses.
- Keep the display focused on the current day/session and next transition.

## Planned modules

- Schedule engine: course day/session map, time ranges, and transitions.
- Settings storage: course start date, course type, seat identifiers.
- Display composition: current session, time remaining, and next location.

## Data flow

- Settings are stored in persistent storage.
- Schedule engine computes "now" and "next" states on each minute tick.
- UI layers render the derived text values.
