// Persistent settings storage and date conversions.
#include "settings.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

enum {
  SETTINGS_KEY_COURSE_START = 1,
  SETTINGS_KEY_SERVICE_START = 2,
  SETTINGS_KEY_COURSE_TYPE = 3,
  SETTINGS_KEY_COURSE_ROLE = 4,
  SETTINGS_KEY_ROOM = 5,
  SETTINGS_KEY_PAGODA = 6,
  SETTINGS_KEY_DINING = 7,
  SETTINGS_KEY_CUSHION = 8,
  SETTINGS_KEY_DEMO_ENABLED = 9,
  SETTINGS_KEY_DEMO_CYCLE_SECONDS = 10,
};

void settings_copy_string(char *dest, size_t dest_size, const char *src) {
  if (!src) {
    dest[0] = '\0';
    return;
  }
  strncpy(dest, src, dest_size - 1);
  dest[dest_size - 1] = '\0';
}

DateTimeParts settings_datetime_from_parts(int ymd, int hm) {
  DateTimeParts date = {
    .year = ymd / 10000,
    .month = (ymd / 100) % 100,
    .day = ymd % 100,
    .hour = hm / 100,
    .minute = hm % 100,
  };
  return date;
}

int settings_date_to_ymd(DateTimeParts date) {
  return date.year * 10000 + date.month * 100 + date.day;
}

int settings_time_to_hm(DateTimeParts date) {
  return date.hour * 100 + date.minute;
}

static int parse_two(const char *value) {
  if (!value || !isdigit((unsigned char)value[0]) || !isdigit((unsigned char)value[1])) {
    return 0;
  }
  return (value[0] - '0') * 10 + (value[1] - '0');
}

static int parse_four(const char *value) {
  if (!value) {
    return 0;
  }
  int result = 0;
  for (int i = 0; i < 4; i++) {
    if (!isdigit((unsigned char)value[i])) {
      return 0;
    }
    result = result * 10 + (value[i] - '0');
  }
  return result;
}

DateTimeParts settings_datetime_from_iso(const char *value) {
  DateTimeParts date = {0};
  if (!value) {
    return date;
  }

  if (strlen(value) < 16) {
    return date;
  }

  date.year = parse_four(value);
  date.month = parse_two(value + 5);
  date.day = parse_two(value + 8);
  date.hour = parse_two(value + 11);
  date.minute = parse_two(value + 14);

  return date;
}

void settings_datetime_to_iso(DateTimeParts date, char *buffer, size_t buffer_size) {
  snprintf(buffer, buffer_size, "%04d-%02d-%02d %02d:%02d",
           date.year, date.month, date.day, date.hour, date.minute);
}

time_t settings_datetime_to_time(DateTimeParts date) {
  struct tm time_parts = {
    .tm_year = date.year - 1900,
    .tm_mon = date.month - 1,
    .tm_mday = date.day,
    .tm_hour = date.hour,
    .tm_min = date.minute,
    .tm_sec = 0,
  };
  return mktime(&time_parts);
}

void settings_datetime_from_time(DateTimeParts *date, time_t timestamp) {
  struct tm *time_parts = localtime(&timestamp);
  if (!time_parts) {
    return;
  }
  date->year = time_parts->tm_year + 1900;
  date->month = time_parts->tm_mon + 1;
  date->day = time_parts->tm_mday;
  date->hour = time_parts->tm_hour;
  date->minute = time_parts->tm_min;
}

void settings_set_defaults(Settings *settings) {
  time_t now = time(NULL);
  DateTimeParts today = {0};
  settings_datetime_from_time(&today, now);

  settings->course_start = today;
  settings->service_start = today;
  settings->course_type = COURSE_TYPE_TEN_DAY;
  settings->course_role = COURSE_ROLE_STUDENT;
  settings_copy_string(settings->room, sizeof(settings->room), "");
  settings_copy_string(settings->pagoda_cell, sizeof(settings->pagoda_cell), "");
  settings_copy_string(settings->dining_hall, sizeof(settings->dining_hall), "");
  settings_copy_string(settings->cushion, sizeof(settings->cushion), "");
  settings->demo_enabled = true;
  settings->demo_cycle_seconds = 1;
}

void settings_load(Settings *settings) {
  settings_set_defaults(settings);

  if (persist_exists(SETTINGS_KEY_COURSE_START)) {
    int ymd = persist_read_int(SETTINGS_KEY_COURSE_START);
    int hm = 0;
    if (persist_exists(SETTINGS_KEY_COURSE_START + 100)) {
      hm = persist_read_int(SETTINGS_KEY_COURSE_START + 100);
    }
    settings->course_start = settings_datetime_from_parts(ymd, hm);
  }

  if (persist_exists(SETTINGS_KEY_SERVICE_START)) {
    int ymd = persist_read_int(SETTINGS_KEY_SERVICE_START);
    int hm = 0;
    if (persist_exists(SETTINGS_KEY_SERVICE_START + 100)) {
      hm = persist_read_int(SETTINGS_KEY_SERVICE_START + 100);
    }
    settings->service_start = settings_datetime_from_parts(ymd, hm);
  }

  if (persist_exists(SETTINGS_KEY_COURSE_TYPE)) {
    settings->course_type = (CourseType)persist_read_int(SETTINGS_KEY_COURSE_TYPE);
  }

  if (persist_exists(SETTINGS_KEY_COURSE_ROLE)) {
    settings->course_role = (CourseRole)persist_read_int(SETTINGS_KEY_COURSE_ROLE);
  }

  if (persist_exists(SETTINGS_KEY_ROOM)) {
    persist_read_string(SETTINGS_KEY_ROOM, settings->room, sizeof(settings->room));
  }

  if (persist_exists(SETTINGS_KEY_PAGODA)) {
    persist_read_string(SETTINGS_KEY_PAGODA, settings->pagoda_cell, sizeof(settings->pagoda_cell));
  }

  if (persist_exists(SETTINGS_KEY_DINING)) {
    persist_read_string(SETTINGS_KEY_DINING, settings->dining_hall, sizeof(settings->dining_hall));
  }

  if (persist_exists(SETTINGS_KEY_CUSHION)) {
    persist_read_string(SETTINGS_KEY_CUSHION, settings->cushion, sizeof(settings->cushion));
  }

  if (persist_exists(SETTINGS_KEY_DEMO_ENABLED)) {
    settings->demo_enabled = persist_read_bool(SETTINGS_KEY_DEMO_ENABLED);
  }

  if (persist_exists(SETTINGS_KEY_DEMO_CYCLE_SECONDS)) {
    settings->demo_cycle_seconds = persist_read_int(SETTINGS_KEY_DEMO_CYCLE_SECONDS);
  }
}

void settings_save(const Settings *settings) {
  persist_write_int(SETTINGS_KEY_COURSE_START, settings_date_to_ymd(settings->course_start));
  persist_write_int(SETTINGS_KEY_COURSE_START + 100, settings_time_to_hm(settings->course_start));
  persist_write_int(SETTINGS_KEY_SERVICE_START, settings_date_to_ymd(settings->service_start));
  persist_write_int(SETTINGS_KEY_SERVICE_START + 100, settings_time_to_hm(settings->service_start));
  persist_write_int(SETTINGS_KEY_COURSE_TYPE, (int)settings->course_type);
  persist_write_int(SETTINGS_KEY_COURSE_ROLE, (int)settings->course_role);
  persist_write_string(SETTINGS_KEY_ROOM, settings->room);
  persist_write_string(SETTINGS_KEY_PAGODA, settings->pagoda_cell);
  persist_write_string(SETTINGS_KEY_DINING, settings->dining_hall);
  persist_write_string(SETTINGS_KEY_CUSHION, settings->cushion);
  persist_write_bool(SETTINGS_KEY_DEMO_ENABLED, settings->demo_enabled);
  persist_write_int(SETTINGS_KEY_DEMO_CYCLE_SECONDS, settings->demo_cycle_seconds);
}
