// Main watchface entrypoint for the Vipassana course schedule display.
#include <pebble.h>
#include <stdio.h>

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
static TextLayer *s_current_layer;
static TextLayer *s_next_layer;
static TextLayer *s_countdown_layer;
static TextLayer *s_mode_layer;

static Settings s_settings;
static BatteryChargeState s_battery_state;

static char s_time_buffer[8];
static char s_battery_buffer[12];
static char s_day_buffer[24];
static char s_current_buffer[48];
static char s_next_buffer[48];
static char s_countdown_buffer[32];
static char s_mode_buffer[32];

static int minutes_from_tm(const struct tm *time_parts) {
  return time_parts->tm_hour * 60 + time_parts->tm_min;
}

static void format_time_hhmm(int minutes, char *buffer, size_t buffer_size) {
  int hours = (minutes / 60) % 24;
  int mins = minutes % 60;
  snprintf(buffer, buffer_size, "%02d:%02d", hours, mins);
}

static int days_between(time_t start, time_t end) {
  time_t start_day = start / 86400;
  time_t end_day = end / 86400;
  return (int)(end_day - start_day);
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
  time_t course_start = settings_date_to_time(s_settings.course_start);
  time_t service_start = settings_date_to_time(s_settings.service_start);

  int days_until = 0;
  AppMode mode = determine_mode(now, service_start, course_start, 11, &days_until);

  if (mode == MODE_COURSE) {
    int course_day = days_between(course_start, now);
    int days_left = 11 - course_day;
    snprintf(s_day_buffer, sizeof(s_day_buffer), "Day %d / 11 (%d left)", course_day, days_left);
    text_layer_set_text(s_day_layer, s_day_buffer);

    DaySchedule schedule = schedule_get_day(s_settings.course_type, s_settings.course_role, course_day);
    int minutes_now = minutes_from_tm(tick_time);
    int current_index = schedule_current_index(&schedule, minutes_now);
    int next_index = schedule_next_index(&schedule, current_index);

    const Activity *current = &schedule.activities[current_index];
    snprintf(s_current_buffer, sizeof(s_current_buffer), "%s", current->label);
    text_layer_set_text(s_current_layer, s_current_buffer);

    DaySchedule next_schedule = schedule;
    if (current_index == (int)schedule.count - 1) {
      next_schedule = schedule_get_day(s_settings.course_type, s_settings.course_role, course_day + 1);
    }
    const Activity *next = &next_schedule.activities[next_index];
    char next_time[8];
    format_time_hhmm(next->minutes, next_time, sizeof(next_time));
    snprintf(s_next_buffer, sizeof(s_next_buffer), "%s %s", next_time, next->label);
    text_layer_set_text(s_next_layer, s_next_buffer);

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
      snprintf(s_countdown_buffer,
               sizeof(s_countdown_buffer),
               "%s in %d min",
               schedule_kind_label(target_kind),
               minutes_until);
    } else {
      snprintf(s_countdown_buffer, sizeof(s_countdown_buffer), "%s soon", schedule_kind_label(target_kind));
    }
    text_layer_set_text(s_countdown_layer, s_countdown_buffer);
    text_layer_set_text(s_mode_layer, "Course");
  } else if (mode == MODE_SERVICE) {
    snprintf(s_mode_buffer, sizeof(s_mode_buffer), "Service Period");
    text_layer_set_text(s_mode_layer, s_mode_buffer);
    snprintf(s_day_buffer, sizeof(s_day_buffer), "Course in %d days", days_until);
    text_layer_set_text(s_day_layer, s_day_buffer);
    text_layer_set_text(s_current_layer, "Service in progress");
    text_layer_set_text(s_next_layer, "Next: course start");
    text_layer_set_text(s_countdown_layer, "");
  } else if (mode == MODE_PRE_SERVICE) {
    snprintf(s_mode_buffer, sizeof(s_mode_buffer), "Pre-service");
    text_layer_set_text(s_mode_layer, s_mode_buffer);
    snprintf(s_day_buffer, sizeof(s_day_buffer), "Service in %d days", days_until);
    text_layer_set_text(s_day_layer, s_day_buffer);
    text_layer_set_text(s_current_layer, "Awaiting service");
    text_layer_set_text(s_next_layer, "");
    text_layer_set_text(s_countdown_layer, "");
  } else if (mode == MODE_PRE_COURSE) {
    snprintf(s_mode_buffer, sizeof(s_mode_buffer), "Pre-course");
    text_layer_set_text(s_mode_layer, s_mode_buffer);
    snprintf(s_day_buffer, sizeof(s_day_buffer), "Course in %d days", days_until);
    text_layer_set_text(s_day_layer, s_day_buffer);
    text_layer_set_text(s_current_layer, "Awaiting course");
    text_layer_set_text(s_next_layer, "");
    text_layer_set_text(s_countdown_layer, "");
  } else {
    text_layer_set_text(s_mode_layer, "Update dates");
    text_layer_set_text(s_day_layer, "Course complete");
    text_layer_set_text(s_current_layer, "Open settings");
    text_layer_set_text(s_next_layer, "");
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

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  settings_ui_show();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, select_click_handler);
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

  s_time_layer = create_label(GRect(0, 2, bounds.size.w, 40), GTextAlignmentCenter,
                              fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  s_battery_layer = create_label(GRect(0, 40, bounds.size.w, 18), GTextAlignmentCenter,
                                 fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));

  s_mode_layer = create_label(GRect(0, 58, bounds.size.w, 18), GTextAlignmentCenter,
                              fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_mode_layer));

  s_day_layer = create_label(GRect(0, 76, bounds.size.w, 20), GTextAlignmentCenter,
                             fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(s_day_layer));

  s_current_layer = create_label(GRect(4, 98, bounds.size.w - 8, 32), GTextAlignmentCenter,
                                 fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_current_layer));

  s_next_layer = create_label(GRect(4, 130, bounds.size.w - 8, 20), GTextAlignmentCenter,
                              fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(s_next_layer));

  s_countdown_layer = create_label(GRect(4, 150, bounds.size.w - 8, 20), GTextAlignmentCenter,
                                   fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(s_countdown_layer));
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_battery_layer);
  text_layer_destroy(s_mode_layer);
  text_layer_destroy(s_day_layer);
  text_layer_destroy(s_current_layer);
  text_layer_destroy(s_next_layer);
  text_layer_destroy(s_countdown_layer);
}

static void init(void) {
  settings_load(&s_settings);
  settings_ui_init(&s_settings, settings_changed);

  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_click_config_provider(s_main_window, click_config_provider);
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

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  settings_ui_deinit();
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
