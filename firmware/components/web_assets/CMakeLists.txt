file(GLOB SOURCES *.c)

idf_component_register(SRCS ${SOURCES}
                    EMBED_FILES index.html.gz favicon.ico)
