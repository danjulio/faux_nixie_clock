file(GLOB SOURCES *.c)

include_directories(../cmd ../lvgl)
add_library(gui STATIC ${SOURCES})
