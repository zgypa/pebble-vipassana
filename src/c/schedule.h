// Schedule engine for course activities and timing calculations.
#pragma once

#include <pebble.h>
#include <stddef.h>
#include <stdint.h>

#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH(a) (sizeof(a) / sizeof((a)[0]))
#endif

typedef enum {
  COURSE_TYPE_TEN_DAY = 0,
} CourseType;

typedef enum {
  COURSE_ROLE_STUDENT = 0,
  COURSE_ROLE_SERVER = 1,
} CourseRole;

typedef enum {
  DAYTYPE_PRE_COURSE = 0,
  DAYTYPE_ARRIVAL,
  DAYTYPE_ANAPANA,
  DAYTYPE_TRANSITION,
  DAYTYPE_VIPASSANA,
  DAYTYPE_METTA,
  DAYTYPE_DEPARTURE,
} DayType;

typedef enum {
  ACTIVITY_MEDITATION = 0,
  ACTIVITY_REST,
  ACTIVITY_WORK,
  ACTIVITY_MEAL,
  ACTIVITY_DISCOURSE,
  ACTIVITY_INFO,
  ACTIVITY_SLEEP,
  ACTIVITY_OTHER,
} ActivityKind;

typedef enum {
  MEDITATION_NONE = 0,
  MEDITATION_ANAPANA,
  MEDITATION_VIPASSANA,
  MEDITATION_METTA,
} MeditationType;

typedef struct {
  uint16_t minutes;
  const char *label;
  ActivityKind kind;
  MeditationType meditation;
} Activity;

typedef struct {
  const Activity *activities;
  size_t count;
} DaySchedule;

DaySchedule schedule_get_day(CourseType course_type, CourseRole role, int day);
DayType schedule_get_day_type(CourseType course_type, int day);
MeditationType schedule_meditation_for_day(DayType day_type);
int schedule_current_index(const DaySchedule *schedule, int minutes);
int schedule_next_index(const DaySchedule *schedule, int current_index);
int schedule_minutes_until_next_kind(const DaySchedule *schedule,
                                     int current_index,
                                     ActivityKind kind,
                                     int minutes_now,
                                     int *out_day_offset);
const char *schedule_kind_label(ActivityKind kind);
const char *schedule_meditation_label(MeditationType meditation);
