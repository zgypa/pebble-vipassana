// Persistent settings and on-watch configuration helpers.
#pragma once

#include <pebble.h>
#include <time.h>

#include "schedule.h"

typedef struct {
  int year;
  int month;
  int day;
} DateParts;

typedef struct {
  DateParts course_start;
  DateParts service_start;
  CourseType course_type;
  CourseRole course_role;
  int pagoda_cell;
  int dining_hall;
  int cushion;
} Settings;

void settings_load(Settings *settings);
void settings_save(const Settings *settings);
void settings_set_defaults(Settings *settings);
DateParts settings_date_from_ymd(int ymd);
int settings_date_to_ymd(DateParts date);
time_t settings_date_to_time(DateParts date);
void settings_date_from_time(DateParts *date, time_t timestamp);
