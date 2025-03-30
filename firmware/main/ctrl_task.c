/*
 * Control Interface Task - Manage platform activities including battery monitoring,
 * shutdown control and persistant storage.
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
#include "ctrl_task.h"
#include "gcore.h"
#include "gui_task.h"
#include "power_utilities.h"
#include "ps_utilities.h"
#include "sntp_utilities.h"
#include "sys_utilities.h"
#include "system_config.h"
#include "web_task.h"
#include "wifi_utilities.h"
#include <string.h>



//
// Variables
//
static const char* TAG = "ctrl_task";

// State
static gui_config_t gui_config;

// Notifications
static bool notify_network_reset = false;
static bool notify_network_restart = false;
static bool notify_shutdown = false;
static bool notify_update_backlight = false;



//
// Forward Declarations for internal functions
//
static void _ctrl_handle_notifications();
static void _ctrl_display_wifi_info();



//
// Control Task API
//
void ctrl_task()
{
	batt_status_t batt_status;
	bool low_batt_msg_displayed = false;
	bool cur_client_connected;
	bool prev_client_connected = false;
	bool cur_wifi_available;
	bool prev_wifi_available = false;
	int batt_sample_count;
	int nvram_save_count;
	
	ESP_LOGI(TAG, "Start task");
	
	// Initialize
	batt_sample_count = CTRL_BATT_SAMPLE_MSEC / CTRL_EVAL_MSEC;
	nvram_save_count = CTRL_NVRAM_SAVE_MSEC / CTRL_EVAL_MSEC;
	ps_get_config(PS_CONFIG_TYPE_GUI, &gui_config);
	
	// Set the initial screen brightness
	power_set_brightness(gui_config.lcd_brightness);
	
	// Set the button for network reset detection
	(void) gcore_set_reg8(GCORE_REG_PWR_TM, NETWORK_RESET_BTN_MSEC/10);
	
	// Initialize power/status monitoring
	if (!power_init()) {
		notify_shutdown = true;
	}
	
	while (1) {
		vTaskDelay(pdMS_TO_TICKS(CTRL_EVAL_MSEC));
		
		// Get notifications
		_ctrl_handle_notifications();
		
		// Evaluate power button every evaluation
		power_status_update();

		if (power_short_button_pressed()) {
			notify_shutdown = true;
		}
		if (power_long_button_pressed()) {
			// Long press resets network to factory default (and restarts it)
			notify_network_reset = true;
			notify_network_restart = true;
		}
		
		// Handle backlight updates
		if (notify_update_backlight) {
			notify_update_backlight = false;
			ps_get_config(PS_CONFIG_TYPE_GUI, &gui_config);
			power_set_brightness(gui_config.lcd_brightness);
		}
	
		// Update battery state
		if (--batt_sample_count == 0) {
			batt_sample_count = CTRL_BATT_SAMPLE_MSEC / CTRL_EVAL_MSEC;
			power_batt_update();
			power_get_batt(&batt_status);
			
			if (batt_status.batt_state >= BATT_25) {
				if (!low_batt_msg_displayed) {
					gui_set_secondary_msg("Low Battery", 0);
					xTaskNotify(task_handle_gui, GUI_NOTIFY_SECONDARY_MESSAGE, eSetBits);
					low_batt_msg_displayed = true;
				}
			} else {
				if (low_batt_msg_displayed) {
					gui_set_secondary_msg("Low Battery", 1);
					xTaskNotify(task_handle_gui, GUI_NOTIFY_SECONDARY_MESSAGE, eSetBits);
					low_batt_msg_displayed = false;
				}
			}
			if (batt_status.batt_state == BATT_CRIT) {
				ESP_LOGI(TAG, "Critical battery voltage detected");
				notify_shutdown = true;
			}
		}
		
		// Handle power off requests
		if (notify_shutdown) {
			xTaskNotify(task_handle_web, WEB_NOTIFY_SHUTDOWN_MASK, eSetBits);
			
			// Reset the button for fast power on
			(void) gcore_set_reg8(GCORE_REG_PWR_TM, 100/10);
			
			// Configure for a automatic power-on on start-of charge for the
			// critical battery shutdown
			if (batt_status.batt_state == BATT_CRIT) {
				(void) gcore_set_reg8(GCORE_REG_WK_CTRL, GCORE_WK_CHRG_START_MASK);
			} else {
				// Disable wake on charge when we've been manually turned off
				(void) gcore_set_reg8(GCORE_REG_WK_CTRL, 0);
			}
			
			ESP_LOGI(TAG, "Shutdown");
			vTaskDelay(pdMS_TO_TICKS(100));
			power_off();
		}
		
		// Periodically write NVRAM to backing flash on gCore EFM8
		if (--nvram_save_count == 0) {
			nvram_save_count = CTRL_NVRAM_SAVE_MSEC / CTRL_EVAL_MSEC;
			ps_save_to_flash(); // Only writes if there are changes
		}
			
		// Get current connectivity status
		if (wifi_is_sta()) {
			cur_wifi_available = wifi_is_connected();
		} else {
			cur_wifi_available = wifi_is_enabled();
		}
		cur_client_connected = web_has_client();
		
		
		// Look for wifi state change and determine if we should enable/disable SNTP
		if (cur_wifi_available) {
			if (!prev_wifi_available) {
				if (wifi_is_sta()) {
					sntp_start_service();
				}
				prev_wifi_available = true;
				
				// Display the new wifi info on the clock
				_ctrl_display_wifi_info();
			}
		} else {
			if (prev_wifi_available) {
				// Always stop SNTP when wifi isn't available (in case we are now AP but were STA)
				sntp_stop_service();
				prev_wifi_available = false;
			}
		}
		
		// Let the user know locally if a client is connected
		if (cur_client_connected) {
			if (!prev_client_connected) {
				gui_set_secondary_msg("Client Connected", 0);
				xTaskNotify(task_handle_gui, GUI_NOTIFY_SECONDARY_MESSAGE, eSetBits);
				prev_client_connected = true;
				low_batt_msg_displayed = false;
			}
		} else {
			if (prev_client_connected) {
				gui_set_secondary_msg("Client Disconnected", 2);
				xTaskNotify(task_handle_gui, GUI_NOTIFY_SECONDARY_MESSAGE, eSetBits);
				prev_client_connected = false;
				low_batt_msg_displayed = false;
			}
		}
		
		// Reset wifi to factory default if requested
		if (notify_network_reset) {
			ESP_LOGI(TAG, "Reset Wi-Fi to factory default");
			ps_reinit_config(PS_CONFIG_TYPE_NET);
			notify_network_reset = false;
		}
		
		// Restart wifi if requested
		if (notify_network_restart) {
			ESP_LOGI(TAG, "Restart Wi-Fi");
			gui_set_secondary_msg("Restarting Wi-Fi...", 2);
			xTaskNotify(task_handle_gui, GUI_NOTIFY_SECONDARY_MESSAGE, eSetBits);
			
			// Let the web task know any clients will be disconnected
			xTaskNotify(task_handle_web, WEB_NOTIFY_NETWORK_DISC_MASK, eSetBits);
			vTaskDelay(pdMS_TO_TICKS(2000));
			
			// Display 
			if (!wifi_reinit()) {
				gui_set_secondary_msg("Wi-Fi failed to restart", 5);
				xTaskNotify(task_handle_gui, GUI_NOTIFY_SECONDARY_MESSAGE, eSetBits);
			}
			
			// Clear our state so we re-display wifi info
			prev_wifi_available = false;
			low_batt_msg_displayed = false;
			notify_network_restart = false;
		}
	}
}


//
// Internal functions
//
static void _ctrl_handle_notifications()
{
	uint32_t notification_value;
	
	notification_value = 0;
	if (xTaskNotifyWait(0x00, 0xFFFFFFFF, &notification_value, 0)) {
	
		if (Notification(notification_value, CTRL_NOTIFY_RESTART_NETWORK)) {
			notify_network_restart = true;
		}
		
		if (Notification(notification_value, CTRL_NOTIFY_SHUTDOWN)) {
			notify_shutdown = true;
		}
		
		if (Notification(notification_value, CTRL_NOTIFY_UPD_BACKLIGHT)) {
			notify_update_backlight = true;
		}
	}
}


static void _ctrl_display_wifi_info()
{
	char buf[48];
	net_config_t wifi_config;
	
	ps_get_config(PS_CONFIG_TYPE_NET, &wifi_config);
	if (wifi_is_sta()) {
		sprintf(buf, "Wi-Fi (STA): %s", wifi_config.sta_ssid);
	} else {
		sprintf(buf, "Wi-Fi (AP): %s", wifi_config.ap_ssid);
	}
	
	gui_set_secondary_msg(buf, 5);
	xTaskNotify(task_handle_gui, GUI_NOTIFY_SECONDARY_MESSAGE, eSetBits);
}