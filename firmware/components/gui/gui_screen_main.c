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
#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gui_screen_main.h"
#include "ps_utilities.h"
#include <stdio.h>
#include <string.h>
#include <time.h>


//
// Constants
//
#define TEXT_COLOR LV_COLOR_MAKE(0xFF, 0xA0, 0x00)



//
// Static images
//
LV_IMG_DECLARE(n1_0_off);
LV_IMG_DECLARE(n1_0);
LV_IMG_DECLARE(n1_1);
LV_IMG_DECLARE(n1_2);
LV_IMG_DECLARE(n1_3);
LV_IMG_DECLARE(n1_4);
LV_IMG_DECLARE(n1_5);
LV_IMG_DECLARE(n1_6);
LV_IMG_DECLARE(n1_7);
LV_IMG_DECLARE(n1_8);
LV_IMG_DECLARE(n1_9);

LV_IMG_DECLARE(c1_0_on);
LV_IMG_DECLARE(c1_0_off);
LV_IMG_DECLARE(c1_1_on);
LV_IMG_DECLARE(c1_1_off);

static const lv_img_dsc_t* digit_images[] = {&n1_0, &n1_1, &n1_2, &n1_3, &n1_4, &n1_5, &n1_6, &n1_7, &n1_8, &n1_9};

// Date related
static const char* days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
static const char* months[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};


//
// Global variables
//

// LVGL objects
static lv_obj_t* main_screen;

static lv_obj_t* canvas_h10;
static lv_obj_t* canvas_h1;
static lv_obj_t* canvas_m10;
static lv_obj_t* canvas_m1;
static lv_obj_t* canvas_c1;
static lv_obj_t* canvas_c2;

static lv_obj_t* lbl_prim_msg;
static lv_obj_t* lbl_sec_msg;

static lv_task_t* task_timer;


// Colon toggle flag
static bool colon_on;

// Previous time digit values, used to determine when to update display
static int prev_h10;
static int prev_h1;
static int prev_m10;
static int prev_m1;
static int prev_day;

// Message timeout 
static int message_prim_timer;
static int message_sec_timer;

// Date string - displayed when there's no primary message
//  "Day Month DOM, Year"
static char date_string[32];



//
// Forward Declarations
//
static void cb_timer(lv_task_t* task);
static void update_time();


//
// API
//
lv_obj_t* gui_screen_main_create()
{
	lv_theme_t* gui_theme;
	
	gui_theme = lv_theme_get_current();
	
	main_screen = lv_obj_create(NULL, NULL);
	lv_obj_set_size(main_screen, LV_HOR_RES_MAX, LV_VER_RES_MAX);
	
	canvas_h10 = lv_canvas_create(main_screen, NULL);
	lv_obj_set_pos(canvas_h10, MAIN_H10_CANVAS_X, MAIN_H10_CANVAS_Y);
	lv_img_set_src(canvas_h10, &n1_0);
	
	canvas_h1= lv_canvas_create(main_screen, NULL);
	lv_obj_set_pos(canvas_h1, MAIN_H1_CANVAS_X, MAIN_H1_CANVAS_Y);
	lv_img_set_src(canvas_h1, &n1_0);
	
	canvas_c1= lv_canvas_create(main_screen, NULL);
	lv_obj_set_pos(canvas_c1, MAIN_C1_CANVAS_X, MAIN_C1_CANVAS_Y);
	lv_img_set_src(canvas_c1, &c1_0_off);
		
	canvas_c2= lv_canvas_create(main_screen, NULL);
	lv_obj_set_pos(canvas_c2, MAIN_C2_CANVAS_X, MAIN_C2_CANVAS_Y);
	lv_img_set_src(canvas_c2, &c1_1_off);
	
	canvas_m10 = lv_canvas_create(main_screen, NULL);
	lv_obj_set_pos(canvas_m10, MAIN_M10_CANVAS_X, MAIN_M10_CANVAS_Y);
	lv_img_set_src(canvas_m10, &n1_0);
	
	canvas_m1 = lv_canvas_create(main_screen, NULL);
	lv_obj_set_pos(canvas_m1, MAIN_M1_CANVAS_X, MAIN_M1_CANVAS_Y);
	lv_img_set_src(canvas_m1, &n1_0);
	
	lbl_prim_msg = lv_label_create(main_screen, NULL);
	lv_label_set_long_mode(lbl_prim_msg, LV_LABEL_LONG_CROP);
	lv_label_set_align(lbl_prim_msg, LV_LABEL_ALIGN_CENTER);
	lv_obj_set_pos(lbl_prim_msg, MAIN_PRIM_MSG_X, MAIN_PRIM_MSG_Y);
	lv_obj_set_size(lbl_prim_msg, MAIN_PRIM_MSG_W, MAIN_PRIM_MSG_H);
	static lv_style_t lbl_prim_style;
	lv_style_copy(&lbl_prim_style, gui_theme->style.bg);
	lbl_prim_style.text.font = &lv_font_roboto_28;
	lbl_prim_style.text.color = TEXT_COLOR;
	lv_label_set_style(lbl_prim_msg, LV_LABEL_STYLE_MAIN, &lbl_prim_style);
	lv_label_set_static_text(lbl_prim_msg, "");
	
	lbl_sec_msg = lv_label_create(main_screen, NULL);
	lv_label_set_long_mode(lbl_sec_msg, LV_LABEL_LONG_CROP);
	lv_label_set_align(lbl_sec_msg, LV_LABEL_ALIGN_CENTER);
	lv_obj_set_pos(lbl_sec_msg, MAIN_SEC_MSG_X, MAIN_SEC_MSG_Y);
	lv_obj_set_size(lbl_sec_msg, MAIN_SEC_MSG_W, MAIN_SEC_MSG_H);
	static lv_style_t lbl_sec_style;
	lv_style_copy(&lbl_sec_style, gui_theme->style.bg);
	lbl_sec_style.text.font = &lv_font_roboto_22;
	lbl_sec_style.text.color = TEXT_COLOR;
	lv_label_set_style(lbl_sec_msg, LV_LABEL_STYLE_MAIN, &lbl_sec_style);
	lv_label_set_static_text(lbl_sec_msg, "");
	
	colon_on = false;
	prev_h10 = -1;
	prev_h1 = -1;
	prev_m10 = -1;
	prev_m1 = -1;
	prev_day = 0;
	message_prim_timer = 0;
	message_sec_timer = 0;
	
	return main_screen;
}


void gui_screen_main_set_active(bool en)
{
	if (en) {
		// Reset state
		colon_on = false;
		prev_h10 = -1;
		prev_h1 = -1;
		prev_m10 = -1;
		prev_m1 = -1;
		prev_day = 0;
		message_prim_timer = 0;
		message_sec_timer = 0;
	
		// Update the time display
		update_time();
		
		// Start our timer task
		task_timer = lv_task_create(cb_timer, 500, LV_TASK_PRIO_LOW, NULL);
	} else {
		if (task_timer != NULL) {
			lv_task_del(task_timer);
			task_timer = NULL;
		}
	}
}


void gui_screen_main_set_prim_msg(const char* msg, int to)
{
	lv_label_set_static_text(lbl_prim_msg, msg);
	message_prim_timer = to * 2;
}


void gui_screen_main_set_sec_msg(const char* msg, int to)
{
	lv_label_set_static_text(lbl_sec_msg, msg);
	message_sec_timer = to * 2;
}



//
// Internal functions
//
static void cb_timer(lv_task_t* task)
{
	// Toggle the colon
	colon_on = !colon_on;
	if (colon_on) {
		lv_img_set_src(canvas_c1, &c1_0_on);
		lv_img_set_src(canvas_c2, &c1_1_on);
	} else {
		lv_img_set_src(canvas_c1, &c1_0_off);
		lv_img_set_src(canvas_c2, &c1_1_off);
	}
		
	// Check if we need to update any time digits
	update_time();
	
	// Check if we need to remove any message
	if (message_prim_timer != 0) {
		if (--message_prim_timer == 0) {
			// Replace message with date
			lv_label_set_static_text(lbl_prim_msg, date_string);
		}
	}
	if (message_sec_timer != 0) {
		if (--message_sec_timer == 0) {
			lv_label_set_static_text(lbl_sec_msg, "");
		}
	}
}


static void update_time()
{
	int cur_h10, cur_h1, cur_m10, cur_m1;
	time_t systime;
	struct tm *now;
	gui_config_t gui_config;
	
	ps_get_config(PS_CONFIG_TYPE_GUI, &gui_config);
	
	systime = time(NULL);
	now = localtime(&systime);
	
	if (!gui_config.hour_mode_24) {
		// Convert 24-hour time to 12-hour time
		if (now->tm_hour > 12) now->tm_hour -= 12;
		
		// Midnight is "12:XX" instead of "00:XX"
		if (now->tm_hour == 0) now->tm_hour = 12;
	}
	
	// Check for time digit updates
	cur_h10 = now->tm_hour / 10;
	cur_h1 = now->tm_hour % 10;
	cur_m10 = now->tm_min / 10;
	cur_m1 = now->tm_min % 10;
	
	if (cur_h10 != prev_h10) {
		if (cur_h10 == 0) {
			// Blank leading zero
			lv_img_set_src(canvas_h10, &n1_0_off);
		} else {
			lv_img_set_src(canvas_h10, digit_images[cur_h10]);
		}
		prev_h10 = cur_h10;
	}
	if (cur_h1 != prev_h1) {
		lv_img_set_src(canvas_h1, digit_images[cur_h1]);
		prev_h1 = cur_h1;
	}
	if (cur_m10 != prev_m10) {
		lv_img_set_src(canvas_m10, digit_images[cur_m10]);
		prev_m10 = cur_m10;
	}
	if (cur_m1 != prev_m1) {
		lv_img_set_src(canvas_m1, digit_images[cur_m1]);
		prev_m1 = cur_m1;
	}
	
	// Check for a date update
	if (prev_day != now->tm_mday) {
		sprintf(date_string, "%s %s %d, %d", days[now->tm_wday], months[now->tm_mon], now->tm_mday, now->tm_year + 1900);
		if (message_prim_timer == 0) {
			lv_label_set_static_text(lbl_prim_msg, date_string);
		}
		prev_day = now->tm_mday;
	}
}
