/*
 * ESP32-specific websocket data handling utilities.
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
#include <arpa/inet.h>
#include "cmd_handlers.h"
#include "cmd_list.h"
#include "cmd_utilities.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/semphr.h"
#include "sys_utilities.h"
#include "ws_cmd_utilities.h"




//
// Local constants
//

// Encoded websocket packet (network byte order)
//   uint32_t  length     (complete packet length)
//   uint32_t  cmd_type
//   uint32_t  cmd_id
//   uint32_t  data_type
//   uint8_t[] data
#define WS_PKT_LEN_OFFSET   0
#define WS_PKT_CTYPE_OFFSET 4
#define WS_PKT_ID_OFFSET    8
#define WS_PKT_DTYPE_OFFSET 12
#define WS_PKT_DATA_OFFSET  16

// Minimum websocket packet size (no data)
#define MIN_WS_PKT_LEN      16

// Maximum websocket packet size
#define MAX_WS_PKT_LEN      (MIN_WS_PKT_LEN + 8192)

// Each TX buffer (sent to gui through websocket) is sized to hold one packet
#define WS_TX_BUFFER_LEN    (MAX_WS_PKT_LEN)

// RX buffer (holds packet received from gui) is sized to hold one packet.
// It's designed to handed to websocket receive code by calling task and
// then processed.
#define WS_RX_BUFFER_LEN    (MIN_WS_PKT_LEN + 8192)

// Maximum number of stored TX packets
#define WS_MAX_TX_PKTS      4



//
// Typedefs
//
typedef struct {
	uint8_t* buf;
	uint32_t len;
} tx_buffer_t;


//
// Local variables
//
static const char* TAG = "ws_cmd_utilities";

// RX buffer for processing a single received web socket packet at a time in a callback
// function called by the underlying web server task.  Its contents should be processed
// immediately (and any response pushed into the TX buffer)
static uint8_t* rx_buffer;

// The TX buffer consists of multiple individual buffers each holding one packet.
// They are treated as a circular buffer and protected with a mutex so the can be
// loaded from both the underlying web server task and our web task.  A bit wasteful
// of space but we're putting them in PSRAM.
static tx_buffer_t tx_buffer[WS_MAX_TX_PKTS];
static int tx_buffer_push_index = 0;
static int tx_buffer_pop_index = 0;
static int tx_buffer_num_entries = 0;
static SemaphoreHandle_t tx_mutex;



//
// API
//
bool ws_gui_cmd_init()
{
	// Allocate rx and tx buffers
	for (int i=0; i<WS_MAX_TX_PKTS; i++) {
		tx_buffer[i].buf =  (uint8_t*) heap_caps_malloc(WS_TX_BUFFER_LEN, MALLOC_CAP_SPIRAM);
		if (tx_buffer[i].buf == NULL) {
			ESP_LOGE(TAG, "create tx_buffer %d failed", i);
			return false;
		}
	}
	tx_mutex = xSemaphoreCreateMutex();
	
	rx_buffer = (uint8_t*) heap_caps_malloc(WS_RX_BUFFER_LEN, MALLOC_CAP_SPIRAM);
	if (rx_buffer == NULL) {
		ESP_LOGE(TAG, "malloc rx_buffer failed");
		return false;
	}
	
	// Initialize the command system
	if (!cmd_init_remote(ws_cmd_send_handler)) {
		return false;
	}
	
	// Register command handlers supported on our end (get, set, rsp)
	(void) cmd_register_cmd_id(CMD_BACKLIGHT, cmd_handler_get_backlight, cmd_handler_set_backlight, NULL);
	(void) cmd_register_cmd_id(CMD_MODE, cmd_handler_get_mode, cmd_handler_set_mode, NULL);
	(void) cmd_register_cmd_id(CMD_POWEROFF, NULL, cmd_handler_set_poweroff, NULL);
	(void) cmd_register_cmd_id(CMD_SYS_INFO, cmd_handler_get_sys_info, NULL, NULL);
	(void) cmd_register_cmd_id(CMD_TIME, cmd_handler_get_time, cmd_handler_set_time, NULL);
	(void) cmd_register_cmd_id(CMD_TIMEZONE, cmd_handler_get_timezone, cmd_handler_set_timezone, NULL);
	(void) cmd_register_cmd_id(CMD_WIFI_INFO, cmd_handler_get_wifi, cmd_handler_set_wifi, NULL);
	
	return true;
}


bool ws_cmd_get_tx_data(uint32_t* len, uint8_t** data)
{
	bool valid;
	
	xSemaphoreTake(tx_mutex, portMAX_DELAY);
	valid = (tx_buffer_num_entries > 0);
	if (valid) {
		*data = tx_buffer[tx_buffer_pop_index].buf;
		*len = tx_buffer[tx_buffer_pop_index].len;
		if (++tx_buffer_pop_index >= WS_MAX_TX_PKTS) tx_buffer_pop_index = 0;
		tx_buffer_num_entries -= 1;
	}
	xSemaphoreGive(tx_mutex);
	
	return valid;
}


uint8_t* ws_cmd_get_rx_data_buffer()
{
	return rx_buffer;
}


bool ws_cmd_process_socket_rx_data(uint32_t len, uint8_t* data)
{
	cmd_t cmd_type;
	cmd_id_t cmd_id;
	cmd_data_t data_type;
	uint32_t dlen;
	
	// Make sure received data contains at least the minimum cmd arguments
	if (len < MIN_WS_PKT_LEN) {
		ESP_LOGE(TAG, "Illegal websocket packet length %lu", len);
		return false;
	}
	
	// Make sure the received data length matches what the cmd says its length is
	dlen = ntohl(*((uint32_t*) &data[WS_PKT_LEN_OFFSET]));
	if (len != dlen) {
		ESP_LOGE(TAG, "websocket packet len %lu does not match expected %lu", len, dlen);
		return false;
	}
	
	// Convert raw packet data in network order to cmd arguments
	cmd_type = (cmd_t) ntohl(*((uint32_t*) &data[WS_PKT_CTYPE_OFFSET]));
	cmd_id = (cmd_id_t) ntohl(*((uint32_t*) &data[WS_PKT_ID_OFFSET]));
	data_type = (cmd_data_t) ntohl(*((uint32_t*) &data[WS_PKT_DTYPE_OFFSET]));
	dlen = len - WS_PKT_DATA_OFFSET;
	return cmd_process_received_cmd(cmd_type, cmd_id, data_type, dlen, data + WS_PKT_DATA_OFFSET);
}


// Encode responses from the command response handlers into our tx_buffer
bool ws_cmd_send_handler(cmd_t cmd_type, cmd_id_t cmd_id, cmd_data_t data_type, uint32_t len, uint8_t* data)
{
	uint32_t* tx32P;
	uint8_t* tx8P;
	
	if (tx_buffer_num_entries == WS_MAX_TX_PKTS) {
		ESP_LOGE(TAG, "TX Buffer full for ws_cmd_send_handler(%d, %d, %d, ...)", (int) cmd_type, (int) cmd_id, (int) data_type);
		return false;
	}
	
	// Atomically get and fill a buffer
	xSemaphoreTake(tx_mutex, portMAX_DELAY);
	tx32P = (uint32_t*) tx_buffer[tx_buffer_push_index].buf;
	tx8P = tx_buffer[tx_buffer_push_index].buf + WS_PKT_DATA_OFFSET;
	tx_buffer[tx_buffer_push_index].len = WS_PKT_DATA_OFFSET + len;
	if (++tx_buffer_push_index == WS_MAX_TX_PKTS) tx_buffer_push_index = 0;
	tx_buffer_num_entries += 1;
	
	// Add the fields, in order, to the websocket packet in network byte order
	*tx32P++ = htonl(WS_PKT_DATA_OFFSET + len);
	*tx32P++ = htonl((uint32_t) cmd_type);
	*tx32P++ = htonl((uint32_t) cmd_id);
	*tx32P   = htonl((uint32_t) data_type);
	
	// Add data if it exists
	while (len--) {
		*tx8P++ = *data++;
	}
	
	xSemaphoreGive(tx_mutex);
	
	return true;
}
