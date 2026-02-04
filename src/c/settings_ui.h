// On-watch settings UI for editing course dates and preferences.
#pragma once

#include <pebble.h>

#include "settings.h"

typedef void (*SettingsChangedHandler)(const Settings *settings);

void settings_ui_init(Settings *settings, SettingsChangedHandler handler);
void settings_ui_deinit(void);
void settings_ui_show(void);
