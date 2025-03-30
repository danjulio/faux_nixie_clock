/*
 * Main GUI screen related functions, callbacks and event handlers
 *
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
#ifndef GUI_SCREEN_MAIN_H
#define GUI_SCREEN_MAIN_H

#include <stdint.h>
#include <stdbool.h>
#include "lvgl/lvgl.h"

//
// Main Screen Constants
//

// Time digits
#define MAIN_H10_CANVAS_X 15
#define MAIN_H10_CANVAS_Y 20
#define MAIN_H10_CANVAS_W 92
#define MAIN_H10_CANVAS_H 166

#define MAIN_H1_CANVAS_X (MAIN_H10_CANVAS_X + MAIN_H10_CANVAS_W + 11)
#define MAIN_H1_CANVAS_Y 20
#define MAIN_H1_CANVAS_W 92
#define MAIN_H1_CANVAS_H 166

#define MAIN_C1_CANVAS_X (MAIN_H1_CANVAS_X + MAIN_H1_CANVAS_W + 15)
#define MAIN_C1_CANVAS_Y (MAIN_H1_CANVAS_Y + 1 * (MAIN_H1_CANVAS_H)/3 - 15)
#define MAIN_C1_CANVAS_W 30
#define MAIN_C1_CANVAS_H 30

#define MAIN_C2_CANVAS_X (MAIN_H1_CANVAS_X + MAIN_H1_CANVAS_W + 15)
#define MAIN_C2_CANVAS_Y (MAIN_H1_CANVAS_Y + 2 * (MAIN_H1_CANVAS_H)/3 - 15)
#define MAIN_C2_CANVAS_W 30
#define MAIN_C2_CANVAS_H 30

#define MAIN_M10_CANVAS_X (MAIN_H1_CANVAS_X + MAIN_H1_CANVAS_W + 60)
#define MAIN_M10_CANVAS_Y 20
#define MAIN_M10_CANVAS_W 92
#define MAIN_M10_CANVAS_H 166

#define MAIN_M1_CANVAS_X (MAIN_M10_CANVAS_X + MAIN_M10_CANVAS_W + 11)
#define MAIN_M1_CANVAS_Y 20
#define MAIN_M1_CANVAS_W 92
#define MAIN_M1_CANVAS_H 166

#define MAIN_PRIM_MSG_X  10
#define MAIN_PRIM_MSG_Y  215
#define MAIN_PRIM_MSG_W  460
#define MAIN_PRIM_MSG_H  30

#define MAIN_SEC_MSG_X  10
#define MAIN_SEC_MSG_Y  270
#define MAIN_SEC_MSG_W  460
#define MAIN_SEC_MSG_H  25


//
// API
//
lv_obj_t* gui_screen_main_create();
void gui_screen_main_set_active(bool en);
void gui_screen_main_set_prim_msg(const char* msg, int to);
void gui_screen_main_set_sec_msg(const char* msg, int to);

#endif /* GUI_SCREEN_MAIN_H */
