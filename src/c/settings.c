// Persistent settings storage and date conversions.
#include "settings.h"

enum {
  SETTINGS_KEY_COURSE_START = 1,
  SETTINGS_KEY_SERVICE_START = 2,
  SETTINGS_KEY_COURSE_TYPE = 3,
  SETTINGS_KEY_COURSE_ROLE = 4,
  SETTINGS_KEY_PAGODA = 5,
  SETTINGS_KEY_DINING = 6,
  SETTINGS_KEY_CUSHION = 7,
};

DateParts settings_date_from_ymd(int ymd) {
  DateParts date = {
    .year = ymd / 10000,
    .month = (ymd / 100) % 100,
    .day = ymd % 100,
  };
  return date;
}

int settings_date_to_ymd(DateParts date) {
  return date.year * 10000 + date.month * 100 + date.day;
}

time_t settings_date_to_time(DateParts date) {
  struct tm time_parts = {
    .tm_year = date.year - 1900,
    .tm_mon = date.month - 1,
    .tm_mday = date.day,
    .tm_hour = 0,
    .tm_min = 0,
    .tm_sec = 0,
  };
  return mktime(&time_parts);
}

void settings_date_from_time(DateParts *date, time_t timestamp) {
  struct tm *time_parts = localtime(&timestamp);
  if (!time_parts) {
    return;
  }
  date->year = time_parts->tm_year + 1900;
  date->month = time_parts->tm_mon + 1;
  date->day = time_parts->tm_mday;
}

void settings_set_defaults(Settings *settings) {
  time_t now = time(NULL);
  DateParts today = {0};
  settings_date_from_time(&today, now);

  settings->course_start = today;
  settings->service_start = today;
  settings->course_type = COURSE_TYPE_TEN_DAY;
  settings->course_role = COURSE_ROLE_STUDENT;
  settings->pagoda_cell = 0;
  settings->dining_hall = 0;
  settings->cushion = 0;
}

void settings_load(Settings *settings) {
  settings_set_defaults(settings);

  if (persist_exists(SETTINGS_KEY_COURSE_START)) {
    int ymd = persist_read_int(SETTINGS_KEY_COURSE_START);
    settings->course_start = settings_date_from_ymd(ymd);
  }

  if (persist_exists(SETTINGS_KEY_SERVICE_START)) {
    int ymd = persist_read_int(SETTINGS_KEY_SERVICE_START);
    settings->service_start = settings_date_from_ymd(ymd);
  }

  if (persist_exists(SETTINGS_KEY_COURSE_TYPE)) {
    settings->course_type = (CourseType)persist_read_int(SETTINGS_KEY_COURSE_TYPE);
  }

  if (persist_exists(SETTINGS_KEY_COURSE_ROLE)) {
    settings->course_role = (CourseRole)persist_read_int(SETTINGS_KEY_COURSE_ROLE);
  }

  if (persist_exists(SETTINGS_KEY_PAGODA)) {
    settings->pagoda_cell = persist_read_int(SETTINGS_KEY_PAGODA);
  }

  if (persist_exists(SETTINGS_KEY_DINING)) {
    settings->dining_hall = persist_read_int(SETTINGS_KEY_DINING);
  }

  if (persist_exists(SETTINGS_KEY_CUSHION)) {
    settings->cushion = persist_read_int(SETTINGS_KEY_CUSHION);
  }
}

void settings_save(const Settings *settings) {
  persist_write_int(SETTINGS_KEY_COURSE_START, settings_date_to_ymd(settings->course_start));
  persist_write_int(SETTINGS_KEY_SERVICE_START, settings_date_to_ymd(settings->service_start));
  persist_write_int(SETTINGS_KEY_COURSE_TYPE, (int)settings->course_type);
  persist_write_int(SETTINGS_KEY_COURSE_ROLE, (int)settings->course_role);
  persist_write_int(SETTINGS_KEY_PAGODA, settings->pagoda_cell);
  persist_write_int(SETTINGS_KEY_DINING, settings->dining_hall);
  persist_write_int(SETTINGS_KEY_CUSHION, settings->cushion);
}
