set(SOURCES ctrl_task.c gui_task.c main.c web_task.c)
idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS .
                    REQUIRES cmd esp_event esp_netif esp_http_server gui i2c lvgl lvgl_tft platform utilities web_assets)

target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_CONF_INCLUDE_SIMPLE=1)