/*
 * System related utilities
 *
 * Contains functions to initialize the system, other utility functions, a set
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
#ifndef SYS_UTILITIES_H
#define SYS_UTILITIES_H

#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "system_config.h"
#include <stdbool.h>
#include <stdint.h>



//
// System Utilities macros
//
#define Notification(var, mask) ((var & mask) == mask)



//
// Task handle externs for use by tasks to communicate with each other
//
extern TaskHandle_t task_handle_ctrl;
extern TaskHandle_t task_handle_gui;
extern TaskHandle_t task_handle_web;

#ifdef INCLUDE_SYS_MON
extern TaskHandle_t task_handle_mon;
#endif



//
// System Utilities API
//
bool system_esp_io_init();
bool system_peripheral_init();
 
#endif /* SYS_UTILITIES_H */