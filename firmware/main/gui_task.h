/*
 * GUI Task - Initialize LVGL, drivers and manage screens for local LCD display.
 *
 * Copyright 2024-2025 Dan Julio
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef _GUI_TASK_H
#define _GUI_TASK_H

#include <stdint.h>
#include <stdbool.h>
#include "freertos/task.h"


//
// Constants
//

// Theme hue (0-360)
#define GUI_THEME_HUE              240

// Screen indicies
#define GUI_SCREEN_MAIN            0
#define GUI_NUM_SCREENS            1

// LVGL evaluation rate (mSec)
#define GUI_EVAL_MSEC              25

//
// GUI Task notifications
//
// From cid_task
#define GUI_NOTIFY_PRIMARY_MESSAGE         0x00000001
#define GUI_NOTIFY_SECONDARY_MESSAGE       0x00000002



//
// API
//
void gui_task(void* args);
void gui_set_screen(int n);
void gui_set_primary_msg(const char* msg, int to);
void gui_set_secondary_msg(const char* msg, int to);

#endif /* _GUI_TASK_H */