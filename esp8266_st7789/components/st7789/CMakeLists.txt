set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR})
set(SRC_FILES
    ${SRC_DIR}/hardware/tft.cpp
    ${SRC_DIR}/graphics/render_glyphs.cpp
)
idf_component_register(
    SRCS ${SRC_FILES}
    INCLUDE_DIRS ${SRC_DIR}
    PRIV_INCLUDE_DIRS ${SRC_DIR}
    PRIV_REQUIRES global_pwm
)
