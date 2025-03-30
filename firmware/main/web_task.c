/*
 *
 * Web Task - web server and associated callbacks
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
 *
 */
#include "esp_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmd_handlers.h"
#include "cmd_list.h"
#include "cmd_utilities.h"
#include "ctrl_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "sys_utilities.h"
#include "ws_cmd_utilities.h"
#include "web_task.h"
#include "wifi_utilities.h"


//
// WEB Task constants
//

// Maximum number of connections
#define max_sockets 3



//
// WEB Task typedefs
//

// Command packet data types
typedef enum {
	SEND_CMD_SHUTDOWN,
} send_cmd_type_t;


//
// WEB Task variables
//
static const char* TAG = "web_task";

// State
static bool client_connected = false;

// Notifications (clear after use)
static bool notify_network_disconnect = false;
static bool notify_shutdown = false;

// served web page and favicon
extern const uint8_t index_html_start[] asm("_binary_index_html_gz_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_gz_end");

extern const uint8_t favicon_ico_start[] asm("_binary_favicon_ico_start");
extern const uint8_t favicon_ico_end[] asm("_binary_favicon_ico_end");



//
// WEB Task Forward Declarations for internal functions
//
static void _web_handle_notifications();
static httpd_handle_t _web_start_webserver(void);
static esp_err_t _web_stop_webserver(httpd_handle_t server);
static void _web_connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void _web_disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static esp_err_t _web_req_handler(httpd_req_t *req);
static esp_err_t _web_favicon_handler(httpd_req_t *req);
static esp_err_t _web_ws_handler(httpd_req_t *req);
static void _web_send_cmd(httpd_handle_t handle, int sock, send_cmd_type_t cmd_type);



//
// Web handler configuration
//
static const httpd_uri_t uri_get = {
        .uri        = "/",
        .method     = HTTP_GET,
        .handler    = _web_req_handler,
        .user_ctx   = NULL,
        .is_websocket = false
};

static const httpd_uri_t uri_get_favicon = {
        .uri        = "/favicon.ico",
        .method     = HTTP_GET,
        .handler    = _web_favicon_handler,
        .user_ctx   = NULL,
        .is_websocket = false
};

static const httpd_uri_t uri_ws = {
        .uri        = "/ws",
        .method     = HTTP_GET,
        .handler    = _web_ws_handler,
        .user_ctx   = NULL,
        .is_websocket = true
};



//
// WEB Task API
//
void web_task()
{
	esp_err_t ret;
	size_t clients;
	int client_fds[max_sockets];
	int sock;
	static httpd_handle_t server = NULL;
	
	ESP_LOGI(TAG, "Start task");
	
	// Initialize cmd interface
	if (!ws_gui_cmd_init()) {
		ESP_LOGE(TAG, "Could not initialize command interface");
		vTaskDelete(NULL);
	}
	
	// Wait until we are connected to start the web server
	while (!wifi_is_connected()) {
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	
	// Register even handlers to stop the server when Wifi is disconnected and
	// start it again upon connection
	(void) esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &_web_connect_handler, &server);
	(void) esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &_web_disconnect_handler, &server);
	
	// Start the server for the first time
	if ((server = _web_start_webserver()) ==  NULL) {
		ESP_LOGE(TAG, "Could not start web server");
		vTaskDelete(NULL);
	}

	
	while (1) {
		_web_handle_notifications();

		// Give the scheduler some time between images
		vTaskDelay(pdMS_TO_TICKS(10));
		
		if (server != NULL) {
			// Look for connectivity changes or messages to send
			clients = max_sockets;
			if ((ret = httpd_get_client_list(server, &clients, client_fds)) == ESP_OK) {
				client_connected = (clients != 0);
				for (int i=0; i<clients; i++) {
					sock = client_fds[i];
					if (httpd_ws_get_fd_info(server, sock) == HTTPD_WS_CLIENT_WEBSOCKET) {
						// Look for things to send
						if (notify_network_disconnect) {
							ret = httpd_sess_trigger_close(server, sock);
							if (ret != ESP_OK) {
								ESP_LOGE(TAG, "Couldn't close connection (%d)", ret);
							}
						}
						
						if (notify_shutdown) {
							_web_send_cmd(server, sock, SEND_CMD_SHUTDOWN);
						}
					}
				}
			} else {
				ESP_LOGE(TAG, "httpd_get_client_list failed (%d)", ret);
			}
		}
		
		// Clear notifications every time through loop to handle case where nothing
		// is connected and we ignore them
		notify_network_disconnect = false;
		notify_shutdown = false;
	}
}


bool web_has_client()
{
	return client_connected;
}



//
// WEB Task Internal functions
//
static void _web_handle_notifications()
{
	uint32_t notification_value;
	
	notification_value = 0;
	if (xTaskNotifyWait(0x00, 0xFFFFFFFF, &notification_value, 0)) {
	
		if (Notification(notification_value, WEB_NOTIFY_NETWORK_DISC_MASK)) {
			notify_network_disconnect = true;
		}
		
		if (Notification(notification_value, WEB_NOTIFY_SHUTDOWN_MASK)) {
			notify_shutdown = true;
		}
	}
}


static httpd_handle_t _web_start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    
    // Setup our specific config items
    config.max_open_sockets = max_sockets;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Registering the ws handler
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_get_favicon);
        httpd_register_uri_handler(server, &uri_ws);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}


static esp_err_t _web_stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    return httpd_stop(server);
}


static void _web_connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = _web_start_webserver();
        if (*server == NULL) {
			ESP_LOGE(TAG, "Could not restart web server");
			vTaskDelete(NULL);
        }
    }
}


static void _web_disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver");
        if (_web_stop_webserver(*server) == ESP_OK) {
            *server = NULL;
        } else {
            ESP_LOGE(TAG, "Failed to stop http server");
        }
    }
}


static esp_err_t _web_req_handler(httpd_req_t *req)
{
	uint32_t len = index_html_end - index_html_start;
	
	ESP_LOGI(TAG, "Sending index.html");
	
	if (httpd_resp_set_hdr(req, "Content-Encoding", "gzip") != ESP_OK) {
		ESP_LOGE(TAG, "set_hdr failed");
		return false;
	}
	
	return httpd_resp_send(req, (const char*) index_html_start, (ssize_t) len);
}


static esp_err_t _web_favicon_handler(httpd_req_t *req)
{
	uint32_t len = favicon_ico_end - favicon_ico_start;
	
	ESP_LOGI(TAG, "Sending favicon");
	
	(void) httpd_resp_set_type(req, "image/x-icon");
	
	return httpd_resp_send(req, (const char*) favicon_ico_start, (ssize_t) len);
}


static esp_err_t _web_ws_handler(httpd_req_t *req)
{
	esp_err_t ret;
	httpd_ws_frame_t ws_pkt;
	
	// Handle opening the websocket
	if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Handshake done, socket opened");
        return ESP_OK;
    }
    
    // Look for incoming packets to process
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_BINARY;
    ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }
    
    if (ws_pkt.len) {
    	// Get and process the websocket packet
    	ws_pkt.payload = ws_cmd_get_rx_data_buffer();
    	ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
            return ret;
        }
        
        // May push response data into the tx buffer
        (void) ws_cmd_process_socket_rx_data(ws_pkt.len, ws_pkt.payload);
        
        // Check for response data (from a GET)
        while (ws_cmd_get_tx_data((uint32_t*) &ws_pkt.len, &ws_pkt.payload)) {
        	// Send the response
        	ws_pkt.type = HTTPD_WS_TYPE_BINARY;
        	ws_pkt.final = true;
			ws_pkt.fragmented = false;
			ret = httpd_ws_send_frame(req, &ws_pkt);
		    if (ret != ESP_OK) {
		        ESP_LOGE(TAG, "httpd_ws_send_frame failed with %d", ret);
		    }
        }
    }
    
    return ESP_OK;
}


static void _web_send_cmd(httpd_handle_t handle, int sock, send_cmd_type_t cmd_type)
{
	esp_err_t ret;
	httpd_ws_frame_t ws_pkt;
	
	if (handle == NULL) return;
	
	// Create the specific command to send
	switch (cmd_type) {
		case SEND_CMD_SHUTDOWN:
			(void) cmd_send(CMD_SET, CMD_SHUTDOWN);
			break;
	}
	
	// Synchronously send the packet
	while (ws_cmd_get_tx_data((uint32_t*) &ws_pkt.len, &ws_pkt.payload)) {
		ws_pkt.type = HTTPD_WS_TYPE_BINARY;
		ws_pkt.final = true;
		ws_pkt.fragmented = false;
		ret = httpd_ws_send_data(handle, sock, &ws_pkt);
		if (ret != ESP_OK) {
			ESP_LOGE(TAG, "httpd_ws_send_data failed - %d", ret);
		}
	}
}
