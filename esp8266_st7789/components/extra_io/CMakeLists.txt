set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(SRC_FILES
    ${SRC_DIR}/extra_button.cpp
    ${SRC_DIR}/extra_led.cpp
)

idf_component_register(
    SRCS ${SRC_FILES}
    INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/include
    PRIV_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/include
    REQUIRES global_pwm
)
