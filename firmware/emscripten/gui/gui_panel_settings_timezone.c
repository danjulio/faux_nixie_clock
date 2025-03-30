/*
 * GUI settings timezone control panel
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
#include "cmd_utilities.h"
#include "gui_main.h"
#include "gui_page_settings.h"
#include "gui_panel_settings_timezone.h"
#include "gui_state.h"
#include <string.h>


//
// Local variables
//

// State
static int cur_timezone_index;
static int orig_timezone_index;

//
// LVGL Objects
//
static lv_obj_t* my_panel;
static lv_obj_t* lbl_name;
static lv_obj_t* rlr_timezone;

// Change detect timer task
static lv_task_t* task_upd_timer;


// Timezone values
#define NUM_TZ_PARM_VALS 20
static const char* parm_tz_list = "ACST/ACDT\n"
                                  "AEST/AEDT\n"
                                  "AKST/AKDT\n"
                                  "ANAT\n"
                                  "AWST\n"
                                  "GMT/BST\n"
                                  "CET/CEST\n"
                                  "CST/CDT\n"
                                  "CST\n"
                                  "EET/EEST\n"
                                  "EST/EDT\n"
                                  "GMT\n"
                                  "HST/HDT\n"
                                  "IST\n"
                                  "JST\n"
                                  "MST/MDT\n"
                                  "MST\n"
                                  "NZST/NZDT\n"
                                  "PST/PDT\n"
                                  "SAST";
static const char* parm_tz_value[] = {"ACST-9:30ACDT,M10.1.0,M4.1.0/3",
                                      "AEST-10AEDT,M10.1.0,M4.1.0/3",
                                      "AKST9AKDT,M3.2.0,M11.1.0",
                                      "ANAT-12",
                                      "AWST-8",
                                      "GMT0BST,M3.5.0/1,M10.5.0",
                                      "CET-1CEST,M3.5.0,M10.5.0/3",
                                      "CST6CDT,M3.2.0,M11.1.0",
                                      "CST-8",
                                      "EET-2EEST,M3.5.0/3,M10.5.0/4",
                                      "EST5EDT,M3.2.0,M11.1.0",
                                      "GMT",
                                      "HST10HDT,M3.2.0,M11.1.0",
                                      "IST-5:30",
                                      "JST-9",
                                      "MST7MDT,M3.2.0,M11.1.0",
                                      "MST7",
                                      "NZST-12NZDT,M9.5.0,M4.1.0/3",
                                      "PST8PDT,M3.2.0,M11.1.0",
                                      "SAST-2"};



//
// Forward declarations for internal functions
//
static void _cb_rlr_timezone(lv_obj_t* obj, lv_event_t event);
static void _task_eval_upd_timer(lv_task_t* task);
static int _timezone_to_rlr_index(const char* tz);



//
// API
//
void gui_panel_settings_timezone_init(lv_obj_t* parent_cont)
{
	// Control panel - width fits parent, height fits contents with padding
	my_panel = lv_cont_create(parent_cont, NULL);
	lv_obj_set_click(my_panel, false);
	lv_obj_set_auto_realign(my_panel, true);
	lv_cont_set_fit2(my_panel, LV_FIT_PARENT, LV_FIT_TIGHT);
	lv_cont_set_layout(my_panel, LV_LAYOUT_PRETTY_MID);
	lv_obj_set_style_local_pad_top(my_panel, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, GUIP_SETTINGS_TOP_PAD);
	lv_obj_set_style_local_pad_bottom(my_panel, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, GUIP_SETTINGS_BTM_PAD);
	lv_obj_set_style_local_pad_left(my_panel, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, GUIP_SETTINGS_LEFT_PAD);
	lv_obj_set_style_local_pad_right(my_panel, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, GUIP_SETTINGS_RIGHT_PAD);
	
	// Panel name
	lbl_name = lv_label_create(my_panel, NULL);
	lv_label_set_static_text(lbl_name, "Timezone");
	
	// Timezone selection roller
	rlr_timezone = lv_roller_create(my_panel, NULL);
	lv_roller_set_options(rlr_timezone, parm_tz_list, LV_ROLLER_MODE_NORMAL);
	lv_roller_set_auto_fit(rlr_timezone, false);
	lv_obj_set_size(rlr_timezone, GUIPN_SETTINGS_TIMEZONE_RLR_W, GUIPN_SETTINGS_TIMEZONE_RLR_H);
	lv_obj_set_style_local_bg_color(rlr_timezone, LV_ROLLER_PART_SELECTED, LV_STATE_DEFAULT, GUI_THEME_RLR_BG_COLOR);
	lv_obj_set_event_cb(rlr_timezone, _cb_rlr_timezone);
    
    // Register with our parent page
	gui_page_settings_register_panel(my_panel, NULL, NULL, NULL);
}


void gui_panel_settings_timezone_set_active(bool is_active)
{
	if (is_active) {
		// Set the current timezone
		cur_timezone_index = _timezone_to_rlr_index(gui_state.timezone);
		orig_timezone_index = cur_timezone_index;
		lv_roller_set_selected(rlr_timezone, (uint16_t) cur_timezone_index, LV_ANIM_OFF);
	}
}



//
// Internal functions
//
static void _cb_rlr_timezone(lv_obj_t* obj, lv_event_t event)
{
	if (event == LV_EVENT_VALUE_CHANGED) {
		cur_timezone_index = (int) lv_roller_get_selected(obj);
		
		// Start or update a timer to update NVS after last change
		if (task_upd_timer == NULL) {
			// Start the timer
			task_upd_timer = lv_task_create(_task_eval_upd_timer, GUIPN_IMAGEC_UPD_MSEC, LV_TASK_PRIO_LOW, NULL);
		} else {
			// Reset timer
			lv_task_reset(task_upd_timer);
		}
	}
}


static void _task_eval_upd_timer(lv_task_t* task)
{
	// Save any changed timezone to NVS
	if (cur_timezone_index != orig_timezone_index) {
		orig_timezone_index = cur_timezone_index;
		strcpy(gui_state.timezone, parm_tz_value[cur_timezone_index]);
		(void) cmd_send_string(CMD_SET, CMD_TIMEZONE, (char*) parm_tz_value[cur_timezone_index]);
	}
	
	// Terminate the timer
	lv_task_del(task_upd_timer);
	task_upd_timer = NULL;
}


static int _timezone_to_rlr_index(const char* tz)
{
	for (int i=0; i<NUM_TZ_PARM_VALS; i++) {
		if (strcmp(parm_tz_value[i], tz) == 0) {
			return i;
		}
	}
	
	return 0;
}
