// Main watchface entrypoint for the Vipassana course schedule display.
#include <pebble.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "message_keys.h"
#include "schedule.h"
#include "settings.h"
#include "settings_ui.h"

typedef enum {
  MODE_PRE_SERVICE = 0,
  MODE_PRE_COURSE = 1,
  MODE_SERVICE = 2,
  MODE_COURSE = 3,
  MODE_POST = 4,
} AppMode;

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_battery_layer;
static TextLayer *s_day_layer;
static TextLayer *s_session_layer;
static TextLayer *s_meditation_layer;
static TextLayer *s_location_layer;
static TextLayer *s_next_layer;
static TextLayer *s_next_location_layer;
static TextLayer *s_countdown_layer;

static Settings s_settings;
static BatteryChargeState s_battery_state;

static char s_time_buffer[16];
static char s_battery_buffer[12];
static char s_day_buffer[24];
static char s_session_buffer[48];
static char s_meditation_buffer[24];
static char s_location_buffer[48];
static char s_next_buffer[48];
static char s_next_location_buffer[48];
static char s_countdown_buffer[32];

static const bool k_settings_only =
#ifdef SETTINGS_ONLY
  true;
#else
  false;
#endif
static bool s_services_started = false;
static bool s_message_open = false;

// Layout configuration. Adjust these to tune line positions and spacing.
static const int kTimeY = 0;
static const int kTimeH = 34;
static const int kBatteryY = 34;
static const int kBatteryH = 24;
static const int kDayY = 58;
static const int kDayH = 16;
static const int kSessionY = 74;
static const int kSessionH = 64;
static const int kMeditationY = 138;
static const int kMeditationH = 0;
static const int kLocationY = 138;
static const int kLocationH = 0;
static const int kNextY = 138;
static const int kNextH = 20;
static const int kNextLocationY = 158;
static const int kNextLocationH = 0;
static const int kCountdownY = 158;
static const int kCountdownH = 0;

static int minutes_from_tm(const struct tm *time_parts) {
  return time_parts->tm_hour * 60 + time_parts->tm_min;
}

static void format_duration_hm(int minutes, char *buffer, size_t buffer_size) {
  if (minutes < 0) {
    buffer[0] = '\0';
    return;
  }
  int hours = minutes / 60;
  int mins = minutes % 60;
  snprintf(buffer, buffer_size, "%02d:%02d", hours, mins);
}

static void format_duration_compact(int minutes, char *buffer, size_t buffer_size) {
  if (minutes < 0) {
    buffer[0] = '\0';
    return;
  }
  int hours = minutes / 60;
  int mins = minutes % 60;
  // Drop leading zeros: "1:30" instead of "01:30"
  snprintf(buffer, buffer_size, "%d:%02d", hours, mins);
}

// Calculate remaining days with decimal fraction
// Working day is 18h (04:00 to 22:00), excluding sleep (22:00 to 04:00)
// Returns -1.0 if target is in the past
static float calculate_remaining_days(int current_day, int current_minutes,
                                      int target_day, int target_minutes) {
  const int kWorkingMinutesPerDay = 18 * 60;  // 1080 minutes
  const int kDayStartMinutes = 4 * 60;        // 04:00
  const int kDayEndMinutes = 22 * 60;         // 22:00
  
  // If we're past the target, return -1
  if (current_day > target_day) {
    return -1.0f;
  }
  if (current_day == target_day && current_minutes >= target_minutes) {
    return -1.0f;
  }
  
  // Calculate working minutes remaining today
  int working_minutes_left_today = 0;
  if (current_minutes < kDayStartMinutes) {
    // Before working hours start - full working day ahead
    working_minutes_left_today = kWorkingMinutesPerDay;
  } else if (current_minutes >= kDayEndMinutes) {
    // After working hours end - no working time left today
    working_minutes_left_today = 0;
  } else {
    // During working hours
    working_minutes_left_today = kDayEndMinutes - current_minutes;
  }
  
  // Calculate working minutes until target
  int working_minutes_to_target = 0;
  if (current_day == target_day) {
    // Same day
    if (target_minutes <= kDayEndMinutes && target_minutes >= kDayStartMinutes) {
      working_minutes_to_target = target_minutes - current_minutes;
      if (current_minutes < kDayStartMinutes) {
        working_minutes_to_target = target_minutes - kDayStartMinutes;
      }
    }
  } else {
    // Multiple days
    int full_days_between = target_day - current_day - 1;
    int target_working_minutes = 0;
    
    if (target_minutes >= kDayStartMinutes && target_minutes <= kDayEndMinutes) {
      target_working_minutes = target_minutes - kDayStartMinutes;
    } else if (target_minutes > kDayEndMinutes) {
      target_working_minutes = kWorkingMinutesPerDay;
    }
    
    working_minutes_to_target = working_minutes_left_today +
                                (full_days_between * kWorkingMinutesPerDay) +
                                target_working_minutes;
  }
  
  // Convert to days with one decimal place
  float remaining_days = (float)working_minutes_to_target / (float)kWorkingMinutesPerDay;
  return remaining_days;
}

// Calculate days between two calendar dates (ignoring time of day)
// Returns positive if end is after start, negative if before
static int days_between_dates(int start_year, int start_month, int start_day,
                               int end_year, int end_month, int end_day) {
  // Create tm structs for both dates at noon to avoid DST issues
  struct tm start_tm = {
    .tm_year = start_year - 1900,
    .tm_mon = start_month - 1,
    .tm_mday = start_day,
    .tm_hour = 12,
    .tm_min = 0,
    .tm_sec = 0,
    .tm_isdst = -1,
  };
  struct tm end_tm = {
    .tm_year = end_year - 1900,
    .tm_mon = end_month - 1,
    .tm_mday = end_day,
    .tm_hour = 12,
    .tm_min = 0,
    .tm_sec = 0,
    .tm_isdst = -1,
  };
  
  time_t start_time = mktime(&start_tm);
  time_t end_time = mktime(&end_tm);
  
  // Divide by 86400 to get day difference
  return (int)((end_time - start_time) / 86400);
}

static void append_location_number(char *buffer, size_t buffer_size, const char *label, const char *value) {
  if (value && value[0] != '\0') {
    snprintf(buffer, buffer_size, "%s %s", label, value);
  } else {
    snprintf(buffer, buffer_size, "%s", label);
  }
}

static void format_location(const Activity *activity, char *buffer, size_t buffer_size) {
  if (strstr(activity->label, "Breakfast") || strstr(activity->label, "Lunch") ||
      strstr(activity->label, "Dinner") || strstr(activity->label, "Tea")) {
    append_location_number(buffer, buffer_size, "Dining Hall", s_settings.dining_hall);
    return;
  }

  if (strstr(activity->label, "Rest") || strstr(activity->label, "Lights out") ||
      strstr(activity->label, "Wake up")) {
    append_location_number(buffer, buffer_size, "Room", s_settings.room);
    return;
  }

  if (strstr(activity->label, "Group") || strstr(activity->label, "Discourse") ||
      strstr(activity->label, "Questions") || strstr(activity->label, "Information") ||
      strstr(activity->label, "Meeting")) {
    append_location_number(buffer, buffer_size, "Dhamma Hall", s_settings.cushion);
    return;
  }

  if (activity->kind == ACTIVITY_MEDITATION || strstr(activity->label, "Meditation") ||
      strstr(activity->label, "Metta")) {
    append_location_number(buffer, buffer_size, "Pagoda", s_settings.pagoda_cell);
    return;
  }

  snprintf(buffer, buffer_size, "Service Area");
}

static void send_settings_to_phone(void) {
  if (!s_message_open) {
    return;
  }
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK || !iter) {
    return;
  }

  char course_buffer[24];
  char service_buffer[24];
  settings_datetime_to_iso(s_settings.course_start, course_buffer, sizeof(course_buffer));
  settings_datetime_to_iso(s_settings.service_start, service_buffer, sizeof(service_buffer));

  dict_write_cstring(iter, MESSAGE_KEY_COURSE_START, course_buffer);
  dict_write_cstring(iter, MESSAGE_KEY_SERVICE_START, service_buffer);
  dict_write_int(iter, MESSAGE_KEY_COURSE_TYPE, &s_settings.course_type, sizeof(int), true);
  dict_write_int(iter, MESSAGE_KEY_COURSE_ROLE, &s_settings.course_role, sizeof(int), true);
  dict_write_cstring(iter, MESSAGE_KEY_ROOM, s_settings.room);
  dict_write_cstring(iter, MESSAGE_KEY_PAGODA_CELL, s_settings.pagoda_cell);
  dict_write_cstring(iter, MESSAGE_KEY_DINING_HALL, s_settings.dining_hall);
  dict_write_cstring(iter, MESSAGE_KEY_CUSHION, s_settings.cushion);
  int demo_enabled_int = s_settings.demo_enabled ? 1 : 0;
  dict_write_int(iter, MESSAGE_KEY_DEMO_ENABLED, &demo_enabled_int, sizeof(int), true);
  dict_write_int(iter, MESSAGE_KEY_DEMO_CYCLE_SECONDS, &s_settings.demo_cycle_seconds, sizeof(int), true);

  app_message_outbox_send();
}

static AppMode determine_mode(int now_year, int now_month, int now_day,
                              DateTimeParts service_start,
                              DateTimeParts course_start,
                              int course_length_days,
                              int *out_days_until) {
  int days_until_service = days_between_dates(now_year, now_month, now_day,
                                               service_start.year, service_start.month, service_start.day);
  int days_until_course = days_between_dates(now_year, now_month, now_day,
                                              course_start.year, course_start.month, course_start.day);
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Mode determination: days_until_service=%d, days_until_course=%d", 
          days_until_service, days_until_course);
  
  // Check if we're in the course (course has started and hasn't ended yet)
  // days_until_course <= 0 means course_start is today or in the past
  if (days_until_course <= 0) {
    int days_since_course_start = -days_until_course;
    if (days_since_course_start <= course_length_days) {
      // We're in the course period
      if (out_days_until) {
        *out_days_until = course_length_days - days_since_course_start;
      }
      return MODE_COURSE;
    }
    // Course has ended
    return MODE_POST;
  }
  
  // Course is in the future (days_until_course > 0)
  // Check if we're in service period (service started but course hasn't)
  if (days_until_service <= 0 && days_until_course > 0) {
    if (out_days_until) {
      *out_days_until = days_until_course;
    }
    return MODE_SERVICE;
  }

  // Both are in the future - show whichever comes first
  if (days_until_service < days_until_course) {
    if (out_days_until) {
      *out_days_until = days_until_service;
    }
    return MODE_PRE_SERVICE;
  } else {
    if (out_days_until) {
      *out_days_until = days_until_course;
    }
    return MODE_PRE_COURSE;
  }
}

static int minutes_until_kind(const DaySchedule *schedule,
                              CourseType course_type,
                              CourseRole role,
                              int day,
                              int current_index,
                              int minutes_now,
                              ActivityKind kind) {
  int day_offset = 0;
  int minutes = schedule_minutes_until_next_kind(schedule, current_index, kind, minutes_now, &day_offset);
  if (minutes >= 0) {
    return minutes;
  }

  DaySchedule next_day_schedule = schedule_get_day(course_type, role, day + 1);
  for (size_t i = 0; i < next_day_schedule.count; i++) {
    if (next_day_schedule.activities[i].kind == kind) {
      return (24 * 60 - minutes_now) + (int)next_day_schedule.activities[i].minutes;
    }
  }

  return -1;
}

// Demo mode scenarios: test cases for different times/days
typedef struct {
  int day_offset;     // Days relative to course start (-1, 0, 1, etc.)
  int hour;
  int minute;
  const char *description;
} DemoScenario;

static const DemoScenario k_demo_scenarios[] = {
  {-1, 14, 0, "Day -1 (pre-course)"},
  {0, 18, 0, "Day 0 arrival, before course starts"},
  {0, 20, 30, "Day 0 after course begins"},
  {1, 2, 21, "Day 1 at 02:21 (midnight bug test)"},
  {1, 4, 5, "Day 1 at 04:05 (wake up)"},
  {1, 6, 30, "Day 1 at 06:30 (breakfast)"},
  {1, 7, 20, "Day 1 at 07:20 (between activities)"},
  {1, 9, 1, "Day 1 at 09:01 (short break)"},
  {4, 14, 0, "Day 4 at 14:00 (Vipassana teaching)"},
  {4, 15, 30, "Day 4 at 15:30 (after teaching)"},
  {10, 10, 15, "Day 10 at 10:15 (Metta day, silence ends)"},
  {11, 7, 0, "Day 11 at 07:00 (departure/cleaning)"},
};
static const int k_demo_scenario_count = sizeof(k_demo_scenarios) / sizeof(k_demo_scenarios[0]);
static int s_demo_current_scenario = 0;
static AppTimer *s_demo_timer = NULL;

static void demo_timer_callback(void *data);

static void update_battery(void) {
  s_battery_state = battery_state_service_peek();
  snprintf(s_battery_buffer, sizeof(s_battery_buffer), "%d%%", s_battery_state.charge_percent);
  text_layer_set_text(s_battery_layer, s_battery_buffer);
}

static void update_display(struct tm *tick_time) {
  update_battery();

  // Demo mode: override time with scenario
  struct tm demo_time;
  if (s_settings.demo_enabled && s_demo_current_scenario < k_demo_scenario_count) {
    const DemoScenario *scenario = &k_demo_scenarios[s_demo_current_scenario];
    
    // Calculate demo date relative to course start
    time_t course_start_time = settings_datetime_to_time(s_settings.course_start);
    struct tm *course_start_tm = localtime(&course_start_time);
    course_start_tm->tm_mday += scenario->day_offset;
    course_start_tm->tm_hour = scenario->hour;
    course_start_tm->tm_min = scenario->minute;
    course_start_tm->tm_sec = 0;
    course_start_tm->tm_isdst = -1;
    mktime(course_start_tm);
    
    demo_time = *course_start_tm;
    tick_time = &demo_time;
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Demo mode: scenario %d/%d - %s", 
            s_demo_current_scenario + 1, k_demo_scenario_count, scenario->description);
  }

  int now_year = tick_time->tm_year + 1900;
  int now_month = tick_time->tm_mon + 1;
  int now_day = tick_time->tm_mday;
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Current date/time: %04d-%02d-%02d %02d:%02d (timezone: %s)", 
          now_year, now_month, now_day, tick_time->tm_hour, tick_time->tm_min, 
          tick_time->tm_zone ? tick_time->tm_zone : "unknown");
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Course start: %04d-%02d-%02d %02d:%02d", 
          s_settings.course_start.year, s_settings.course_start.month, s_settings.course_start.day,
          s_settings.course_start.hour, s_settings.course_start.minute);

  int days_until = 0;
  AppMode mode = determine_mode(now_year, now_month, now_day,
                                s_settings.service_start, s_settings.course_start, 11, &days_until);

  if (mode == MODE_COURSE) {
    int course_day = -days_between_dates(now_year, now_month, now_day,
                                          s_settings.course_start.year, 
                                          s_settings.course_start.month,
                                          s_settings.course_start.day);
    int days_left = 11 - course_day;

    DayType day_type = schedule_get_day_type(s_settings.course_type, course_day);
    DaySchedule schedule = schedule_get_day(s_settings.course_type, s_settings.course_role, course_day);
    int minutes_now = minutes_from_tm(tick_time);
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Day: %d, Minutes now: %d, Schedule count: %d", 
            course_day, minutes_now, (int)schedule.count);
    
    int current_index = schedule_current_index(&schedule, minutes_now);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Current index: %d, Activity: %s at %d min", 
            current_index, schedule.activities[current_index].label, 
            schedule.activities[current_index].minutes);
    
    int next_index = schedule_next_index(&schedule, current_index);

    bool before_first_activity = minutes_now < schedule.activities[0].minutes;
    DaySchedule next_schedule = schedule;
    if (current_index == (int)schedule.count - 1 && !before_first_activity) {
      next_schedule = schedule_get_day(s_settings.course_type, s_settings.course_role, course_day + 1);
    }
    const Activity *next = &next_schedule.activities[next_index];
    int minutes_until_next = next->minutes - minutes_now;
    if (!before_first_activity && (current_index == (int)schedule.count - 1 || minutes_until_next < 0)) {
      minutes_until_next = (24 * 60 - minutes_now) + next->minutes;
    }

    // Calculate decimal remaining days countdown
    // Noble silence ends at day 10, 10:10 (610 minutes) when Metta meditation ends
    // Course ends at day 11, 08:50 (530 minutes - when bus leaves)
    const int kNobleSilenceEndDay = 10;
    const int kNobleSilenceEndMinutes = 10 * 60 + 10;  // 10:10
    const int kCourseEndDay = 11;
    const int kCourseEndMinutes = 8 * 60 + 50;  // 08:50
    
    float remaining_days_decimal = -1.0f;
    bool noble_silence_ended = (course_day > kNobleSilenceEndDay) || 
                                (course_day == kNobleSilenceEndDay && minutes_now >= kNobleSilenceEndMinutes);
    
    if (!noble_silence_ended) {
      // Before noble silence ends - countdown to end of noble silence
      remaining_days_decimal = calculate_remaining_days(course_day, minutes_now, 
                                                         kNobleSilenceEndDay, kNobleSilenceEndMinutes);
    } else {
      // After noble silence ends - countdown to course end
      remaining_days_decimal = calculate_remaining_days(course_day, minutes_now, 
                                                         kCourseEndDay, kCourseEndMinutes);
    }
    
    // Top line: "X/-Y.Z H:MM" where X=current day, -Y.Z=remaining days (negative with one decimal), H:MM=time to next
    char duration_buffer[16];
    format_duration_compact(minutes_until_next, duration_buffer, sizeof(duration_buffer));
    
    if (remaining_days_decimal >= 0.0f) {
      // Pebble SDK doesn't support floating point in snprintf, so format manually
      int whole = (int)remaining_days_decimal;
      int decimal = (int)((remaining_days_decimal - (float)whole) * 10.0f + 0.5f);
      if (decimal >= 10) {
        whole += 1;
        decimal = 0;
      }
      snprintf(s_time_buffer, sizeof(s_time_buffer), "%d/-%d.%d %s", course_day, whole, decimal, duration_buffer);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Minutes until next: %d, Duration: '%s', Day: %d, Remaining: -%d.%d", 
              minutes_until_next, duration_buffer, course_day, whole, decimal);
    } else {
      // Fallback to old format if calculation fails
      snprintf(s_time_buffer, sizeof(s_time_buffer), "%d/%d %s", course_day, days_left, duration_buffer);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Minutes until next: %d, Duration: '%s' (fallback mode)", 
              minutes_until_next, duration_buffer);
    }
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Top line text: '%s'", s_time_buffer);
    text_layer_set_text(s_time_layer, s_time_buffer);

    // Day layer is now unused (day info moved to time layer)
    text_layer_set_text(s_day_layer, "");

    const Activity *current = &schedule.activities[current_index];
    snprintf(s_session_buffer, sizeof(s_session_buffer), "%s", current->label);
    text_layer_set_text(s_session_layer, s_session_buffer);

    // Hide meditation type and location
    text_layer_set_text(s_meditation_layer, "");
    text_layer_set_text(s_location_layer, "");

    // Next activity - just the name, no "Next in HH:MM"
    snprintf(s_next_buffer, sizeof(s_next_buffer), "%s", next->label);
    text_layer_set_text(s_next_layer, s_next_buffer);

    // Hide next location and countdown
    text_layer_set_text(s_next_location_layer, "");
    text_layer_set_text(s_countdown_layer, "");
  } else if (mode == MODE_SERVICE) {
    // Calculate minutes until course start
    int days_diff = days_between_dates(now_year, now_month, now_day,
                                        s_settings.course_start.year,
                                        s_settings.course_start.month,
                                        s_settings.course_start.day);
    int minutes_until_course = days_diff * 24 * 60;
    format_duration_hm(minutes_until_course, s_time_buffer, sizeof(s_time_buffer));
    text_layer_set_text(s_time_layer, s_time_buffer);
    char course_buffer[24];
    settings_datetime_to_iso(s_settings.course_start, course_buffer, sizeof(course_buffer));
    snprintf(s_day_buffer, sizeof(s_day_buffer), "Service period");
    text_layer_set_text(s_day_layer, s_day_buffer);
    snprintf(s_day_buffer, sizeof(s_day_buffer), "Course in %d days", days_until);
    text_layer_set_text(s_day_layer, s_day_buffer);
    text_layer_set_text(s_session_layer, "Service in progress");
    snprintf(s_next_buffer, sizeof(s_next_buffer), "Next %s", course_buffer);
    text_layer_set_text(s_next_layer, s_next_buffer);
    text_layer_set_text(s_meditation_layer, "");
    text_layer_set_text(s_location_layer, "");
    text_layer_set_text(s_next_location_layer, "");
    text_layer_set_text(s_countdown_layer, "");
  } else if (mode == MODE_PRE_SERVICE) {
    // Calculate minutes until service start
    int days_diff = days_between_dates(now_year, now_month, now_day,
                                        s_settings.service_start.year,
                                        s_settings.service_start.month,
                                        s_settings.service_start.day);
    int minutes_until_service = days_diff * 24 * 60;
    format_duration_hm(minutes_until_service, s_time_buffer, sizeof(s_time_buffer));
    text_layer_set_text(s_time_layer, s_time_buffer);
    char service_buffer[24];
    settings_datetime_to_iso(s_settings.service_start, service_buffer, sizeof(service_buffer));
    snprintf(s_day_buffer, sizeof(s_day_buffer), "Pre-service");
    text_layer_set_text(s_day_layer, s_day_buffer);
    snprintf(s_day_buffer, sizeof(s_day_buffer), "Service in %d days", days_until);
    text_layer_set_text(s_day_layer, s_day_buffer);
    text_layer_set_text(s_session_layer, "Awaiting service");
    snprintf(s_next_buffer, sizeof(s_next_buffer), "Next %s", service_buffer);
    text_layer_set_text(s_next_layer, s_next_buffer);
    text_layer_set_text(s_meditation_layer, "");
    text_layer_set_text(s_location_layer, "");
    text_layer_set_text(s_next_location_layer, "");
    text_layer_set_text(s_countdown_layer, "");
  } else if (mode == MODE_PRE_COURSE) {
    // Calculate minutes until course start
    int days_diff = days_between_dates(now_year, now_month, now_day,
                                        s_settings.course_start.year,
                                        s_settings.course_start.month,
                                        s_settings.course_start.day);
    int minutes_until_course = days_diff * 24 * 60;
    format_duration_hm(minutes_until_course, s_time_buffer, sizeof(s_time_buffer));
    text_layer_set_text(s_time_layer, s_time_buffer);
    char course_buffer[24];
    settings_datetime_to_iso(s_settings.course_start, course_buffer, sizeof(course_buffer));
    snprintf(s_day_buffer, sizeof(s_day_buffer), "Pre-course");
    text_layer_set_text(s_day_layer, s_day_buffer);
    snprintf(s_day_buffer, sizeof(s_day_buffer), "Course in %d days", days_until);
    text_layer_set_text(s_day_layer, s_day_buffer);
    text_layer_set_text(s_session_layer, "Awaiting course");
    snprintf(s_next_buffer, sizeof(s_next_buffer), "Next %s", course_buffer);
    text_layer_set_text(s_next_layer, s_next_buffer);
    text_layer_set_text(s_meditation_layer, "");
    text_layer_set_text(s_location_layer, "");
    text_layer_set_text(s_next_location_layer, "");
    text_layer_set_text(s_countdown_layer, "");
  } else {
    text_layer_set_text(s_time_layer, "--:--");
    text_layer_set_text(s_day_layer, "Update dates");
    text_layer_set_text(s_day_layer, "Course complete");
    text_layer_set_text(s_session_layer, "Open settings");
    text_layer_set_text(s_next_layer, "");
    text_layer_set_text(s_meditation_layer, "");
    text_layer_set_text(s_location_layer, "");
    text_layer_set_text(s_next_location_layer, "");
    text_layer_set_text(s_countdown_layer, "");
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  // In demo mode, ignore system tick
  if (!s_settings.demo_enabled) {
    update_display(tick_time);
  }
}

static void demo_timer_callback(void *data) {
  if (!s_settings.demo_enabled) {
    s_demo_timer = NULL;
    return;
  }
  
  // Advance to next scenario
  s_demo_current_scenario = (s_demo_current_scenario + 1) % k_demo_scenario_count;
  
  // Update display with current scenario
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  if (tick_time) {
    update_display(tick_time);
  }
  
  // Schedule next demo cycle
  s_demo_timer = app_timer_register(s_settings.demo_cycle_seconds * 1000, demo_timer_callback, NULL);
}

static void battery_handler(BatteryChargeState state) {
  s_battery_state = state;
  update_battery();
}

static void settings_changed(const Settings *settings) {
  bool demo_was_enabled = s_settings.demo_enabled;
  s_settings = *settings;
  
  // Start or stop demo timer based on setting
  if (s_settings.demo_enabled && !demo_was_enabled) {
    // Start demo mode
    s_demo_current_scenario = 0;
    if (s_demo_timer) {
      app_timer_cancel(s_demo_timer);
    }
    s_demo_timer = app_timer_register(100, demo_timer_callback, NULL);
  } else if (!s_settings.demo_enabled && demo_was_enabled) {
    // Stop demo mode
    if (s_demo_timer) {
      app_timer_cancel(s_demo_timer);
      s_demo_timer = NULL;
    }
  }
  
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  if (tick_time) {
    update_display(tick_time);
  }
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *course = dict_find(iter, MESSAGE_KEY_COURSE_START);
  Tuple *service = dict_find(iter, MESSAGE_KEY_SERVICE_START);
  Tuple *course_type = dict_find(iter, MESSAGE_KEY_COURSE_TYPE);
  Tuple *course_role = dict_find(iter, MESSAGE_KEY_COURSE_ROLE);
  Tuple *room = dict_find(iter, MESSAGE_KEY_ROOM);
  Tuple *pagoda = dict_find(iter, MESSAGE_KEY_PAGODA_CELL);
  Tuple *dining = dict_find(iter, MESSAGE_KEY_DINING_HALL);
  Tuple *cushion = dict_find(iter, MESSAGE_KEY_CUSHION);
  Tuple *demo_enabled = dict_find(iter, MESSAGE_KEY_DEMO_ENABLED);
  Tuple *demo_cycle = dict_find(iter, MESSAGE_KEY_DEMO_CYCLE_SECONDS);
  Tuple *request_sync = dict_find(iter, MESSAGE_KEY_REQUEST_SYNC);

  bool demo_changed = false;
  bool demo_was_enabled = s_settings.demo_enabled;

  if (course && course->value->cstring[0] != '\0') {
    s_settings.course_start = settings_datetime_from_iso(course->value->cstring);
  }
  if (service && service->value->cstring[0] != '\0') {
    s_settings.service_start = settings_datetime_from_iso(service->value->cstring);
  }
  if (course_type) {
    s_settings.course_type = (CourseType)course_type->value->int32;
  }
  if (course_role) {
    s_settings.course_role = (CourseRole)course_role->value->int32;
  }
  if (room) {
    settings_copy_string(s_settings.room, sizeof(s_settings.room), room->value->cstring);
  }
  if (pagoda) {
    settings_copy_string(s_settings.pagoda_cell, sizeof(s_settings.pagoda_cell), pagoda->value->cstring);
  }
  if (dining) {
    settings_copy_string(s_settings.dining_hall, sizeof(s_settings.dining_hall), dining->value->cstring);
  }
  if (cushion) {
    settings_copy_string(s_settings.cushion, sizeof(s_settings.cushion), cushion->value->cstring);
  }
  if (demo_enabled) {
    s_settings.demo_enabled = demo_enabled->value->int32 != 0;
    demo_changed = true;
  }
  if (demo_cycle) {
    s_settings.demo_cycle_seconds = demo_cycle->value->int32;
    if (s_settings.demo_cycle_seconds < 1) {
      s_settings.demo_cycle_seconds = 1;
    }
  }

  if (course || service || course_type || course_role || room || pagoda || dining || cushion || demo_enabled || demo_cycle) {
    settings_save(&s_settings);
    
    // Handle demo mode start/stop
    if (demo_changed) {
      if (s_settings.demo_enabled && !demo_was_enabled) {
        // Start demo mode
        s_demo_current_scenario = 0;
        if (s_demo_timer) {
          app_timer_cancel(s_demo_timer);
        }
        s_demo_timer = app_timer_register(100, demo_timer_callback, NULL);
      } else if (!s_settings.demo_enabled && demo_was_enabled) {
        // Stop demo mode
        if (s_demo_timer) {
          app_timer_cancel(s_demo_timer);
          s_demo_timer = NULL;
        }
      }
    }
    
    time_t now = time(NULL);
    struct tm *tick_time = localtime(&now);
    if (tick_time) {
      update_display(tick_time);
    }
  }

  if (request_sync) {
    send_settings_to_phone();
  }
}

static TextLayer *create_label(GRect frame, GTextAlignment alignment, GFont font) {
  TextLayer *layer = text_layer_create(frame);
  text_layer_set_background_color(layer, GColorClear);
  text_layer_set_text_color(layer, GColorBlack);
  text_layer_set_text_alignment(layer, alignment);
  text_layer_set_font(layer, font);
  text_layer_set_overflow_mode(layer, GTextOverflowModeTrailingEllipsis);
  return layer;
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_time_layer = create_label(GRect(0, kTimeY, bounds.size.w, kTimeH), GTextAlignmentCenter,
                              fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  s_battery_layer = create_label(GRect(0, kBatteryY, bounds.size.w, kBatteryH), GTextAlignmentCenter,
                                 fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));

  s_day_layer = create_label(GRect(0, kDayY, bounds.size.w, kDayH), GTextAlignmentCenter,
                             fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(window_layer, text_layer_get_layer(s_day_layer));

  s_session_layer = create_label(GRect(4, kSessionY, bounds.size.w - 8, kSessionH), GTextAlignmentCenter,
                                 fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_session_layer));

  // Meditation, location, next location, and countdown layers hidden (height 0)
  s_meditation_layer = create_label(GRect(4, kMeditationY, bounds.size.w - 8, kMeditationH),
                                    GTextAlignmentCenter, fonts_get_system_font(FONT_KEY_GOTHIC_14));

  s_location_layer = create_label(GRect(4, kLocationY, bounds.size.w - 8, kLocationH),
                                  GTextAlignmentCenter, fonts_get_system_font(FONT_KEY_GOTHIC_14));

  s_next_layer = create_label(GRect(4, kNextY, bounds.size.w - 8, kNextH),
                              GTextAlignmentCenter, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_next_layer));

  s_next_location_layer = create_label(GRect(4, kNextLocationY, bounds.size.w - 8, kNextLocationH),
                                       GTextAlignmentCenter, fonts_get_system_font(FONT_KEY_GOTHIC_14));

  s_countdown_layer = create_label(GRect(4, kCountdownY, bounds.size.w - 8, kCountdownH),
                                   GTextAlignmentCenter, fonts_get_system_font(FONT_KEY_GOTHIC_14));
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_battery_layer);
  text_layer_destroy(s_day_layer);
  text_layer_destroy(s_session_layer);
  text_layer_destroy(s_meditation_layer);
  text_layer_destroy(s_location_layer);
  text_layer_destroy(s_next_layer);
  text_layer_destroy(s_next_location_layer);
  text_layer_destroy(s_countdown_layer);
}

static void init(void) {
  settings_load(&s_settings);

  if (k_settings_only) {
    settings_ui_init(&s_settings, settings_changed);
    settings_ui_show();
    return;
  }

  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorWhite);
  window_set_window_handlers(s_main_window, (WindowHandlers){
                                               .load = main_window_load,
                                               .unload = main_window_unload,
                                           });
  window_stack_push(s_main_window, true);

  update_battery();
  battery_state_service_subscribe(battery_handler);

  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  if (tick_time) {
    update_display(tick_time);
  }

  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(512, 256);
  s_message_open = true;
  send_settings_to_phone();

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  s_services_started = true;
  
  // Start demo mode if enabled
  if (s_settings.demo_enabled) {
    s_demo_current_scenario = 0;
    s_demo_timer = app_timer_register(100, demo_timer_callback, NULL);
  }
}

static void deinit(void) {
  if (s_demo_timer) {
    app_timer_cancel(s_demo_timer);
    s_demo_timer = NULL;
  }
  if (s_services_started) {
    tick_timer_service_unsubscribe();
    battery_state_service_unsubscribe();
  }
  if (k_settings_only) {
    settings_ui_deinit();
  }
  if (s_main_window) {
    window_destroy(s_main_window);
  }
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
