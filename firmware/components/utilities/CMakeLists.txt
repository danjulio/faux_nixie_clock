file(GLOB SOURCES *.c)

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS . ../cmd  ../i2c ../../main ../platform
                       REQUIRES app_update espressif__mdns esp_event esp_netif esp_app_format esp_driver_gpio esp_wifi nvs_flash i2c platform)

