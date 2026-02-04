// On-watch settings UI implementation.
#include "settings_ui.h"

#include <stdio.h>
#include <stdlib.h>

typedef enum {
  EDITOR_NONE = 0,
  EDITOR_COURSE_DATE,
  EDITOR_SERVICE_DATE,
  EDITOR_ROOM,
  EDITOR_PAGODA,
  EDITOR_DINING,
  EDITOR_CUSHION,
} EditorType;

static Settings *s_settings;
static SettingsChangedHandler s_handler;

static Window *s_menu_window;
static SimpleMenuLayer *s_menu_layer;
static SimpleMenuSection s_menu_sections[1];
static SimpleMenuItem s_menu_items[8];

static Window *s_editor_window;
static TextLayer *s_editor_title;
static TextLayer *s_editor_value;
static EditorType s_editor_type;

static char s_course_date_buffer[24];
static char s_service_date_buffer[24];
static char s_role_buffer[16];
static char s_course_type_buffer[16];
static char s_room_buffer[16];
static char s_pagoda_buffer[16];
static char s_dining_buffer[16];
static char s_cushion_buffer[16];

static void format_date(DateTimeParts date, char *buffer, size_t buffer_size) {
  settings_datetime_to_iso(date, buffer, buffer_size);
}

static int parse_int(const char *value) {
  if (!value || value[0] == '\0') {
    return 0;
  }
  return atoi(value);
}

static void format_int(char *buffer, size_t buffer_size, int value) {
  snprintf(buffer, buffer_size, "%d", value);
}

static void refresh_menu(void) {
  format_date(s_settings->course_start, s_course_date_buffer, sizeof(s_course_date_buffer));
  format_date(s_settings->service_start, s_service_date_buffer, sizeof(s_service_date_buffer));
  snprintf(s_role_buffer, sizeof(s_role_buffer), "%s",
           s_settings->course_role == COURSE_ROLE_SERVER ? "Server" : "Student");
  snprintf(s_course_type_buffer, sizeof(s_course_type_buffer), "10-day");
  settings_copy_string(s_room_buffer, sizeof(s_room_buffer), s_settings->room);
  settings_copy_string(s_pagoda_buffer, sizeof(s_pagoda_buffer), s_settings->pagoda_cell);
  settings_copy_string(s_dining_buffer, sizeof(s_dining_buffer), s_settings->dining_hall);
  settings_copy_string(s_cushion_buffer, sizeof(s_cushion_buffer), s_settings->cushion);

  s_menu_items[0] = (SimpleMenuItem){
    .title = "Course Start",
    .subtitle = s_course_date_buffer,
  };
  s_menu_items[1] = (SimpleMenuItem){
    .title = "Service Start",
    .subtitle = s_service_date_buffer,
  };
  s_menu_items[2] = (SimpleMenuItem){
    .title = "Role",
    .subtitle = s_role_buffer,
  };
  s_menu_items[3] = (SimpleMenuItem){
    .title = "Course Type",
    .subtitle = s_course_type_buffer,
  };
  s_menu_items[4] = (SimpleMenuItem){
    .title = "Room",
    .subtitle = s_room_buffer,
  };
  s_menu_items[5] = (SimpleMenuItem){
    .title = "Pagoda Cell",
    .subtitle = s_pagoda_buffer,
  };
  s_menu_items[6] = (SimpleMenuItem){
    .title = "Dining Hall",
    .subtitle = s_dining_buffer,
  };
  s_menu_items[7] = (SimpleMenuItem){
    .title = "Cushion",
    .subtitle = s_cushion_buffer,
  };

  layer_mark_dirty(simple_menu_layer_get_layer(s_menu_layer));
}

static void save_and_notify(void) {
  settings_save(s_settings);
  if (s_handler) {
    s_handler(s_settings);
  }
}

static void editor_update_text(void) {
  if (s_editor_type == EDITOR_COURSE_DATE) {
    format_date(s_settings->course_start, s_course_date_buffer, sizeof(s_course_date_buffer));
    text_layer_set_text(s_editor_title, "Course Start");
    text_layer_set_text(s_editor_value, s_course_date_buffer);
  } else if (s_editor_type == EDITOR_SERVICE_DATE) {
    format_date(s_settings->service_start, s_service_date_buffer, sizeof(s_service_date_buffer));
    text_layer_set_text(s_editor_title, "Service Start");
    text_layer_set_text(s_editor_value, s_service_date_buffer);
  } else if (s_editor_type == EDITOR_ROOM) {
    settings_copy_string(s_room_buffer, sizeof(s_room_buffer), s_settings->room);
    text_layer_set_text(s_editor_title, "Room");
    text_layer_set_text(s_editor_value, s_room_buffer);
  } else if (s_editor_type == EDITOR_PAGODA) {
    settings_copy_string(s_pagoda_buffer, sizeof(s_pagoda_buffer), s_settings->pagoda_cell);
    text_layer_set_text(s_editor_title, "Pagoda Cell");
    text_layer_set_text(s_editor_value, s_pagoda_buffer);
  } else if (s_editor_type == EDITOR_DINING) {
    settings_copy_string(s_dining_buffer, sizeof(s_dining_buffer), s_settings->dining_hall);
    text_layer_set_text(s_editor_title, "Dining Hall");
    text_layer_set_text(s_editor_value, s_dining_buffer);
  } else if (s_editor_type == EDITOR_CUSHION) {
    settings_copy_string(s_cushion_buffer, sizeof(s_cushion_buffer), s_settings->cushion);
    text_layer_set_text(s_editor_title, "Cushion");
    text_layer_set_text(s_editor_value, s_cushion_buffer);
  }
}

static void editor_adjust(int direction) {
  if (s_editor_type == EDITOR_COURSE_DATE || s_editor_type == EDITOR_SERVICE_DATE) {
    DateTimeParts *target = s_editor_type == EDITOR_COURSE_DATE ? &s_settings->course_start
                                                                : &s_settings->service_start;
    time_t base = settings_datetime_to_time(*target);
    base += direction * 86400;
    settings_datetime_from_time(target, base);
  } else if (s_editor_type == EDITOR_ROOM) {
    int value = parse_int(s_settings->room) + direction;
    format_int(s_settings->room, sizeof(s_settings->room), value);
  } else if (s_editor_type == EDITOR_PAGODA) {
    int value = parse_int(s_settings->pagoda_cell) + direction;
    format_int(s_settings->pagoda_cell, sizeof(s_settings->pagoda_cell), value);
  } else if (s_editor_type == EDITOR_DINING) {
    int value = parse_int(s_settings->dining_hall) + direction;
    format_int(s_settings->dining_hall, sizeof(s_settings->dining_hall), value);
  } else if (s_editor_type == EDITOR_CUSHION) {
    int value = parse_int(s_settings->cushion) + direction;
    format_int(s_settings->cushion, sizeof(s_settings->cushion), value);
  }

  editor_update_text();
}

static void editor_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  save_and_notify();
  refresh_menu();
  window_stack_pop(true);
}

static void editor_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  editor_adjust(1);
}

static void editor_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  editor_adjust(-1);
}

static void editor_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, editor_select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, editor_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, editor_down_click_handler);
}

static void editor_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_editor_title = text_layer_create(GRect(0, 16, bounds.size.w, 30));
  text_layer_set_text_alignment(s_editor_title, GTextAlignmentCenter);
  text_layer_set_font(s_editor_title, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_background_color(s_editor_title, GColorClear);
  text_layer_set_text_color(s_editor_title, GColorWhite);
  layer_add_child(window_layer, text_layer_get_layer(s_editor_title));

  s_editor_value = text_layer_create(GRect(0, 54, bounds.size.w, 40));
  text_layer_set_text_alignment(s_editor_value, GTextAlignmentCenter);
  text_layer_set_font(s_editor_value, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_background_color(s_editor_value, GColorClear);
  text_layer_set_text_color(s_editor_value, GColorWhite);
  layer_add_child(window_layer, text_layer_get_layer(s_editor_value));

  editor_update_text();
}

static void editor_window_unload(Window *window) {
  text_layer_destroy(s_editor_title);
  text_layer_destroy(s_editor_value);
}

static void open_editor(EditorType type) {
  s_editor_type = type;
  if (!s_editor_window) {
    s_editor_window = window_create();
    window_set_background_color(s_editor_window, GColorBlack);
    window_set_click_config_provider(s_editor_window, editor_click_config_provider);
    window_set_window_handlers(s_editor_window, (WindowHandlers){
                                                   .load = editor_window_load,
                                                   .unload = editor_window_unload,
                                               });
  }
  window_stack_push(s_editor_window, true);
}

static void menu_select_handler(int index, void *context) {
  switch (index) {
    case 0:
      open_editor(EDITOR_COURSE_DATE);
      break;
    case 1:
      open_editor(EDITOR_SERVICE_DATE);
      break;
    case 2:
      s_settings->course_role = s_settings->course_role == COURSE_ROLE_SERVER
                                   ? COURSE_ROLE_STUDENT
                                   : COURSE_ROLE_SERVER;
      save_and_notify();
      refresh_menu();
      break;
    case 3:
      s_settings->course_type = COURSE_TYPE_TEN_DAY;
      save_and_notify();
      refresh_menu();
      break;
    case 4:
      open_editor(EDITOR_ROOM);
      break;
    case 5:
      open_editor(EDITOR_PAGODA);
      break;
    case 6:
      open_editor(EDITOR_DINING);
      break;
    case 7:
      open_editor(EDITOR_CUSHION);
      break;
    default:
      break;
  }
}

static void menu_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  for (int i = 0; i < (int)ARRAY_LENGTH(s_menu_items); i++) {
    s_menu_items[i].callback = menu_select_handler;
  }

  s_menu_sections[0] = (SimpleMenuSection){
    .items = s_menu_items,
    .num_items = ARRAY_LENGTH(s_menu_items),
  };

  s_menu_layer = simple_menu_layer_create(bounds, window, s_menu_sections, 1, NULL);
  layer_add_child(window_layer, simple_menu_layer_get_layer(s_menu_layer));

  refresh_menu();
}

static void menu_window_unload(Window *window) {
  simple_menu_layer_destroy(s_menu_layer);
}

void settings_ui_init(Settings *settings, SettingsChangedHandler handler) {
  s_settings = settings;
  s_handler = handler;
  s_menu_window = window_create();
  window_set_background_color(s_menu_window, GColorBlack);
  window_set_window_handlers(s_menu_window, (WindowHandlers){
                                               .load = menu_window_load,
                                               .unload = menu_window_unload,
                                           });
}

void settings_ui_deinit(void) {
  if (s_editor_window) {
    window_destroy(s_editor_window);
    s_editor_window = NULL;
  }
  window_destroy(s_menu_window);
}

void settings_ui_show(void) {
  window_stack_push(s_menu_window, true);
}
