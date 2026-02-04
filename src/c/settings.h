// Persistent settings and on-watch configuration helpers.
#pragma once

#include <pebble.h>
#include <time.h>

#include "schedule.h"

typedef struct {
  int year;
  int month;
  int day;
  int hour;
  int minute;
} DateTimeParts;

typedef struct {
  DateTimeParts course_start;
  DateTimeParts service_start;
  CourseType course_type;
  CourseRole course_role;
  char room[8];
  char pagoda_cell[8];
  char dining_hall[8];
  char cushion[8];
} Settings;

void settings_load(Settings *settings);
void settings_save(const Settings *settings);
void settings_set_defaults(Settings *settings);
void settings_copy_string(char *dest, size_t dest_size, const char *src);
DateTimeParts settings_datetime_from_iso(const char *value);
void settings_datetime_to_iso(DateTimeParts date, char *buffer, size_t buffer_size);
int settings_date_to_ymd(DateTimeParts date);
int settings_time_to_hm(DateTimeParts date);
DateTimeParts settings_datetime_from_parts(int ymd, int hm);
time_t settings_datetime_to_time(DateTimeParts date);
void settings_datetime_from_time(DateTimeParts *date, time_t timestamp);
