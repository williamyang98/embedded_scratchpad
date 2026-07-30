set(SRC_FILES
    "main.cpp"
    "websocket_handler.cpp"
    "wifi_sta.c"
)

idf_component_register(
    SRCS ${SRC_FILES}
    INCLUDE_DIRS "."
)
