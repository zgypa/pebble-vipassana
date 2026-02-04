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
static TextLayer *s_mode_layer;

static Settings s_settings;
static BatteryChargeState s_battery_state;

static char s_time_buffer[8];
static char s_battery_buffer[12];
static char s_day_buffer[24];
static char s_session_buffer[48];
static char s_meditation_buffer[24];
static char s_location_buffer[48];
static char s_next_buffer[48];
static char s_next_location_buffer[48];
static char s_countdown_buffer[32];
static char s_mode_buffer[32];

static const bool k_settings_only =
#ifdef SETTINGS_ONLY
  true;
#else
  false;
#endif
static bool s_services_started = false;
static bool s_message_open = false;

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

static int days_between(time_t start, time_t end) {
  time_t start_day = start / 86400;
  time_t end_day = end / 86400;
  return (int)(end_day - start_day);
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

  app_message_outbox_send();
}

static AppMode determine_mode(time_t now,
                              time_t service_start,
                              time_t course_start,
                              int course_length_days,
                              int *out_days_until) {
  time_t earliest = service_start < course_start ? service_start : course_start;
  if (now < earliest) {
    if (out_days_until) {
      *out_days_until = days_between(now, earliest);
    }
    return earliest == service_start ? MODE_PRE_SERVICE : MODE_PRE_COURSE;
  }

  if (now >= service_start && now < course_start) {
    if (out_days_until) {
      *out_days_until = days_between(now, course_start);
    }
    return MODE_SERVICE;
  }

  time_t course_end = course_start + course_length_days * 86400;
  if (now >= course_start && now <= course_end) {
    if (out_days_until) {
      *out_days_until = days_between(now, course_end);
    }
    return MODE_COURSE;
  }

  return MODE_POST;
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

static void update_battery(void) {
  s_battery_state = battery_state_service_peek();
  snprintf(s_battery_buffer, sizeof(s_battery_buffer), "Battery %d%%", s_battery_state.charge_percent);
  text_layer_set_text(s_battery_layer, s_battery_buffer);
}

static void update_display(struct tm *tick_time) {
  strftime(s_time_buffer, sizeof(s_time_buffer), "%H:%M", tick_time);
  text_layer_set_text(s_time_layer, s_time_buffer);

  update_battery();

  time_t now = mktime(tick_time);
  time_t course_start = settings_datetime_to_time(s_settings.course_start);
  time_t service_start = settings_datetime_to_time(s_settings.service_start);

  int days_until = 0;
  AppMode mode = determine_mode(now, service_start, course_start, 11, &days_until);

  if (mode == MODE_COURSE) {
    int course_day = days_between(course_start, now);
    int days_left = 11 - course_day;
    snprintf(s_day_buffer, sizeof(s_day_buffer), "Day %d / 11 (%d left)", course_day, days_left);
    text_layer_set_text(s_day_layer, s_day_buffer);

    DayType day_type = schedule_get_day_type(s_settings.course_type, course_day);
    DaySchedule schedule = schedule_get_day(s_settings.course_type, s_settings.course_role, course_day);
    int minutes_now = minutes_from_tm(tick_time);
    int current_index = schedule_current_index(&schedule, minutes_now);
    int next_index = schedule_next_index(&schedule, current_index);

    const Activity *current = &schedule.activities[current_index];
    snprintf(s_session_buffer, sizeof(s_session_buffer), "%s", current->label);
    text_layer_set_text(s_session_layer, s_session_buffer);

    if (current->kind == ACTIVITY_MEDITATION) {
      MeditationType meditation = current->meditation != MEDITATION_NONE
                                      ? current->meditation
                                      : schedule_meditation_for_day(day_type);
      snprintf(s_meditation_buffer, sizeof(s_meditation_buffer), "%s", schedule_meditation_label(meditation));
      text_layer_set_text(s_meditation_layer, s_meditation_buffer);
    } else {
      text_layer_set_text(s_meditation_layer, "");
    }

    format_location(current, s_location_buffer, sizeof(s_location_buffer));
    text_layer_set_text(s_location_layer, s_location_buffer);

    DaySchedule next_schedule = schedule;
    if (current_index == (int)schedule.count - 1) {
      next_schedule = schedule_get_day(s_settings.course_type, s_settings.course_role, course_day + 1);
    }
    const Activity *next = &next_schedule.activities[next_index];
    int minutes_until_next = next->minutes - minutes_now;
    if (current_index == (int)schedule.count - 1 || minutes_until_next < 0) {
      minutes_until_next = (24 * 60 - minutes_now) + next->minutes;
    }
    char duration_buffer[8];
    format_duration_hm(minutes_until_next, duration_buffer, sizeof(duration_buffer));
    snprintf(s_next_buffer, sizeof(s_next_buffer), "Next in %s %s", duration_buffer, next->label);
    text_layer_set_text(s_next_layer, s_next_buffer);

    format_location(next, s_next_location_buffer, sizeof(s_next_location_buffer));
    text_layer_set_text(s_next_location_layer, s_next_location_buffer);

    ActivityKind target_kind = ACTIVITY_MEDITATION;
    if (current->kind == ACTIVITY_MEDITATION) {
      target_kind = ACTIVITY_REST;
    } else if (current->kind == ACTIVITY_REST) {
      target_kind = ACTIVITY_MEDITATION;
    }

    int minutes_until = minutes_until_kind(&schedule,
                                           s_settings.course_type,
                                           s_settings.course_role,
                                           course_day,
                                           current_index,
                                           minutes_now,
                                           target_kind);
    if (minutes_until >= 0) {
      char duration_buffer[8];
      format_duration_hm(minutes_until, duration_buffer, sizeof(duration_buffer));
      snprintf(s_countdown_buffer,
               sizeof(s_countdown_buffer),
               "%s in %s",
               schedule_kind_label(target_kind),
               duration_buffer);
    } else {
      snprintf(s_countdown_buffer, sizeof(s_countdown_buffer), "%s soon", schedule_kind_label(target_kind));
    }
    text_layer_set_text(s_countdown_layer, s_countdown_buffer);
    text_layer_set_text(s_mode_layer, "Course");
  } else if (mode == MODE_SERVICE) {
    char course_buffer[24];
    settings_datetime_to_iso(s_settings.course_start, course_buffer, sizeof(course_buffer));
    snprintf(s_mode_buffer, sizeof(s_mode_buffer), "Service Period");
    text_layer_set_text(s_mode_layer, s_mode_buffer);
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
    char service_buffer[24];
    settings_datetime_to_iso(s_settings.service_start, service_buffer, sizeof(service_buffer));
    snprintf(s_mode_buffer, sizeof(s_mode_buffer), "Pre-service");
    text_layer_set_text(s_mode_layer, s_mode_buffer);
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
    char course_buffer[24];
    settings_datetime_to_iso(s_settings.course_start, course_buffer, sizeof(course_buffer));
    snprintf(s_mode_buffer, sizeof(s_mode_buffer), "Pre-course");
    text_layer_set_text(s_mode_layer, s_mode_buffer);
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
    text_layer_set_text(s_mode_layer, "Update dates");
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
  update_display(tick_time);
}

static void battery_handler(BatteryChargeState state) {
  s_battery_state = state;
  update_battery();
}

static void settings_changed(const Settings *settings) {
  s_settings = *settings;
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
  Tuple *request_sync = dict_find(iter, MESSAGE_KEY_REQUEST_SYNC);

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

  if (course || service || course_type || course_role || room || pagoda || dining || cushion) {
    settings_save(&s_settings);
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
  text_layer_set_text_color(layer, GColorWhite);
  text_layer_set_text_alignment(layer, alignment);
  text_layer_set_font(layer, font);
  return layer;
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_time_layer = create_label(GRect(0, 2, bounds.size.w, 36), GTextAlignmentCenter,
                              fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  s_battery_layer = create_label(GRect(0, 38, bounds.size.w, 18), GTextAlignmentCenter,
                                 fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));

  s_mode_layer = create_label(GRect(0, 56, bounds.size.w, 18), GTextAlignmentCenter,
                              fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_mode_layer));

  s_day_layer = create_label(GRect(0, 74, bounds.size.w, 20), GTextAlignmentCenter,
                             fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(s_day_layer));

  s_session_layer = create_label(GRect(4, 96, bounds.size.w - 8, 26), GTextAlignmentCenter,
                                 fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_session_layer));

  s_meditation_layer = create_label(GRect(4, 120, bounds.size.w - 8, 20), GTextAlignmentCenter,
                                    fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(s_meditation_layer));

  s_location_layer = create_label(GRect(4, 140, bounds.size.w - 8, 20), GTextAlignmentCenter,
                                  fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(s_location_layer));

  s_next_layer = create_label(GRect(4, 160, bounds.size.w - 8, 20), GTextAlignmentCenter,
                              fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(s_next_layer));

  s_next_location_layer = create_label(GRect(4, 178, bounds.size.w - 8, 20), GTextAlignmentCenter,
                                       fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(s_next_location_layer));

  s_countdown_layer = create_label(GRect(4, 196, bounds.size.w - 8, 20), GTextAlignmentCenter,
                                   fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(s_countdown_layer));
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_battery_layer);
  text_layer_destroy(s_mode_layer);
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
  window_set_background_color(s_main_window, GColorBlack);
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
}

static void deinit(void) {
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
