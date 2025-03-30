/*
 * System related utilities
 *
 * Contains functions to initialize the system, other utility functions and a set
 * of globally available handles for the various tasks (to use for task notifications).
 *
 * Copyright 2020-2025 Dan Julio
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
 *
 */
#include "ctrl_task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "i2c.h"
#include "ps_utilities.h"
#include "sys_utilities.h"
#include "time_utilities.h"
#include "wifi_utilities.h"
#include "system_config.h"
#include <string.h>


//
// System Utilities internal constants
//



//
// System Utilities variables
//
static const char* TAG = "sys";


//
// Task handle externs for use by tasks to communicate with each other
//
TaskHandle_t task_handle_ctrl;
TaskHandle_t task_handle_gui;
TaskHandle_t task_handle_web;

#ifdef INCLUDE_SYS_MON
TaskHandle_t task_handle_mon;
#endif



//
// System Utilities API
//

/**
 * Initialize the ESP32 GPIO and internal peripherals
 */
bool system_esp_io_init()
{
	esp_err_t ret;
	
	ESP_LOGI(TAG, "ESP32 Peripheral Initialization");	
	
	// Attempt to initialize the I2C Master
	ret = i2c_init(I2C_MASTER_SCL_IO, I2C_MASTER_SDA_IO);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "I2C Master initialization failed - %d", ret);
		return false;
	}

	return true;
}


/**
 * Initialize the board-level peripheral subsystems
 */
bool system_peripheral_init()
{
	tz_config_t tz_config;
	
	ESP_LOGI(TAG, "System Peripheral Initialization");
	
	if (!ps_init()) {
		ESP_LOGE(TAG, "Persistent Storage initialization failed");
		return false;
	}
	
	ps_get_config(PS_CONFIG_TYPE_TZ, &tz_config);
	time_init(tz_config.tz);
	
	if (!wifi_init()) {
		ESP_LOGE(TAG, "Wi-Fi initialization failed");
		return false;
	}
	
	return true;
}
