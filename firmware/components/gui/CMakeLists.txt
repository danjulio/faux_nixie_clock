file(GLOB SOURCES *.c gui_assets/*.c)

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS . ../lvgl ../../main ../platform)