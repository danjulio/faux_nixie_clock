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
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_freertos_hooks.h"
#include "gui_task.h"
#include "disp_spi.h"
#include "disp_driver.h"
#include "gui_screen_main.h"
#include "lvgl/lvgl.h"
#include <string.h>


//
// Constants
//

#define MAX_MSG_LEN 80

// Notification decode macro
#define Notification(var, mask) ((var & mask) == mask)



//
// Global variables
//
static const char* TAG = "gui_task";

// Theme
lv_theme_t* gui_theme;

// Dual display update buffers to allow DMA/SPI transfer of one while the other is updated
static lv_color_t lvgl_disp_buf1[LVGL_DISP_BUF_SIZE];
static lv_color_t lvgl_disp_buf2[LVGL_DISP_BUF_SIZE];
static lv_disp_buf_t lvgl_disp_buf;

// Display driver
static lv_disp_drv_t lvgl_disp_drv;

// Screen object array and current screen index
static lv_obj_t* gui_screens[GUI_NUM_SCREENS];
static int gui_cur_screen_index = -1;

// LVGL sub-task 
static lv_task_t* lvgl_task_handle;

// Caller ID messages
static char primary_message[MAX_MSG_LEN+1];
static int primary_message_to;
static char secondary_message[MAX_MSG_LEN+1];
static int secondary_message_to;



//
// Forward declarations
//
static bool gui_lvgl_init();
static void gui_theme_init();
static void gui_screen_init();
static void gui_add_subtasks();
static void gui_task_event_handler_task(lv_task_t * task);
static void lv_tick_callback();




//
// API
//
void gui_task(void* args)
{
	ESP_LOGI(TAG, "Start task");

	// Initialize
	if (!gui_lvgl_init()) {
		vTaskDelete(NULL);
	}
	gui_theme_init();
	gui_screen_init();
	gui_add_subtasks();
	
	// Set the initially displayed screen
	gui_set_screen(GUI_SCREEN_MAIN);
	
	while (1) {
		// This task runs every GUI_EVAL_MSEC mSec
		vTaskDelay(pdMS_TO_TICKS(GUI_EVAL_MSEC));
		lv_task_handler();
	}
}


void gui_set_screen(int n)
{
	if (n < GUI_NUM_SCREENS) {
		gui_cur_screen_index = n;
		
		gui_screen_main_set_active(n == GUI_SCREEN_MAIN);
		
		lv_scr_load(gui_screens[n]);
	}
}


void gui_set_primary_msg(const char* msg, int to)
{
	strncpy(primary_message, msg, MAX_MSG_LEN);
	
	primary_message_to = to;
}


void gui_set_secondary_msg(const char* msg, int to)
{
	strncpy(secondary_message, msg, MAX_MSG_LEN);
	
	secondary_message_to = to;
}




//
// Internal functions
//
static bool gui_lvgl_init()
{
	// Initialize lvgl
	lv_init();
	
	//
	// Interface and driver initialization
	//
	disp_driver_init(true);
	
	// Install the display driver
	lv_disp_buf_init(&lvgl_disp_buf, lvgl_disp_buf1, lvgl_disp_buf2, LVGL_DISP_BUF_SIZE);
	lv_disp_drv_init(&lvgl_disp_drv);
	lvgl_disp_drv.flush_cb = disp_driver_flush;
	lvgl_disp_drv.buffer = &lvgl_disp_buf;
	lv_disp_drv_register(&lvgl_disp_drv);
    
    // Hook LittleVGL's timebase to its CPU system tick so it can keep track of time
    esp_register_freertos_tick_hook(lv_tick_callback);
    
    return true;
}


static void gui_theme_init()
{
	gui_theme = lv_theme_night_init(GUI_THEME_HUE, NULL);
	gui_theme->style.scr->body.main_color = lv_color_hsv_to_rgb(GUI_THEME_HUE, 0, 0);
	gui_theme->style.scr->body.grad_color = lv_color_hsv_to_rgb(GUI_THEME_HUE, 0, 0);
	lv_theme_set_current(gui_theme);
}


static void gui_screen_init()
{
	// Initialize the screens
	gui_screens[GUI_SCREEN_MAIN] = gui_screen_main_create();
}


static void gui_add_subtasks()
{
	// Event handler sub-task runs every GUI_EVAL_MSEC mSec
	lvgl_task_handle = lv_task_create(gui_task_event_handler_task, GUI_EVAL_MSEC,
		LV_TASK_PRIO_MID, NULL);
}


static void gui_task_event_handler_task(lv_task_t * task)
{
	uint32_t notification_value;
	
	// Look for incoming notifications (clear them upon reading)
	if (xTaskNotifyWait(0x00, 0xFFFFFFFF, &notification_value, 0)) {
		if (Notification(notification_value, GUI_NOTIFY_PRIMARY_MESSAGE)) {
			gui_screen_main_set_prim_msg(primary_message, primary_message_to);
		}
		
		if (Notification(notification_value, GUI_NOTIFY_SECONDARY_MESSAGE)) {
			gui_screen_main_set_sec_msg(secondary_message, secondary_message_to);
		}
	}
}


static void IRAM_ATTR lv_tick_callback()
{
	lv_tick_inc(portTICK_PERIOD_MS);
}
