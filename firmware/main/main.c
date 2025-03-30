/*
 * Faux nixie clock on a 480x320 pixel LCD with Wi-Fi time correction and web interface
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
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sys_utilities.h"
#include "ctrl_task.h"
#include "gui_task.h"
#include "web_task.h"



static const char* TAG = "main";



void app_main(void)
{
	ESP_LOGI(TAG, "Faux Nixie Clock starting");
	
	// Initialize system-level ESP32 internal peripherals
    if (!system_esp_io_init()) {
    	ESP_LOGE(TAG, "ESP32 init failed");
    	while (1) {vTaskDelay(pdMS_TO_TICKS(100));}
    }
    
    // Initialize system-level subsystems
    if (!system_peripheral_init()) {
    	ESP_LOGE(TAG, "Peripheral init failed");
    	while (1) {vTaskDelay(pdMS_TO_TICKS(100));}
    }
	
	// Start the tasks that actually comprise the application
	//   Core 0 : PRO
    //   Core 1 : APP
    //
    xTaskCreatePinnedToCore(&ctrl_task, "ctrl_task", 2560, NULL, 2, &task_handle_ctrl, 1);
    xTaskCreatePinnedToCore(&gui_task,  "gui_task",  2560, NULL, 2, &task_handle_gui,  1);
    xTaskCreatePinnedToCore(&web_task,  "web_task",  4096, NULL, 2, &task_handle_web,  0);
}
