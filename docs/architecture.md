# Architecture

This document explains the planned components and data flow for the watchface.

## Goals

- Provide a minimal, distraction-free watchface for Vipassana courses.
- Keep the display focused on the current day/session and next transition.

## Planned modules

- Course composition engine: maps course types to ordered day types.
- Schedule engine: day-type timetables, time ranges, and transitions.
- Settings sync: mobile config (PebbleKit JS) + watch persistence.
- Display composition: current session, meditation type, locations, and countdowns.

## Data flow

- Phone config submits settings via AppMessage and the watch persists them.
- Watch syncs stored settings back to the phone on startup.
- Schedule engine computes "now" and "next" states on each minute tick.
- UI layers render the derived text values.

## Course composition and Day types

- Each course is composed of day types. For example, on a regular 10 day course:
  - Day 0: Arrival
  - Days 1-3: Anapana
  - Day 4: Transition
  - Days 5-9: Vipassana
  - Day 10: Metta
  - Day 11: Departure
- Longer courses extend these sequences by repeating day types.
- The course type "database" is a set of day-type sequences paired with role-specific timetables.
