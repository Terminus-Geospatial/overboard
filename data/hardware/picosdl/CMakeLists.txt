# PicoSDL hardware configuration
# This file contains SDL-specific build settings for the PicoCalc simulator target.
# It is included by the main CMakeLists.txt when TARGET_DEVICE=PICOSDL.
# Uses same config as SDL but with 320x320 screen.

# SDL2 and PNG are required for the simulator
find_package(SDL2 REQUIRED)
find_package(PNG REQUIRED)

# LVGL SDL2 include fix
if(NOT SDL2_INCLUDE_DIRS)
    get_target_property(SDL2_INCLUDE_DIRS SDL2::SDL2 INTERFACE_INCLUDE_DIRECTORIES)
endif()
if(SDL2_INCLUDE_DIRS)
    target_include_directories(lvgl SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS})
endif()

# Set up SDL-specific compile definitions
set(HARDWARE_COMPILE_DEFINITIONS
    TARGET_SDL=1
    TARGET_PICOSDL=1
    SIMULATOR=1
    DEFAULT_CONFIG_PATH="data/configs/picocalc/"
    EMBEDDED_JSON=1
)

# SDL-specific libraries
set(HARDWARE_LIBRARIES
    ${SDL2_LIBRARIES}
    PNG::PNG
)

# SDL-specific include directories
set(HARDWARE_INCLUDE_DIRS
    ${SDL2_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIRS}
)

# Display dimensions for the PicoSDL simulator (320x320)
# Main window is LCD only; keyboard lives in a separate window.
# KBD_HEIGHT=0 so LCD_HEIGHT = FULL_HEIGHT - 0 = 320 (fills the main window).
set(DISPLAY_FULL_WIDTH  320)
set(DISPLAY_FULL_HEIGHT 320)
set(DISPLAY_KBD_WIDTH   420)
set(DISPLAY_KBD_HEIGHT       0)
set(DISPLAY_KBD_WIN_HEIGHT 400)

set(DISPLAY_PREVIEW_MAX_WIDTH  304)
set(DISPLAY_PREVIEW_MAX_HEIGHT 100)

set(DISPLAY_KEY_PAD       2)
set(DISPLAY_KBD_HEADER_H 20)
set(DISPLAY_KBD_MARGIN_L  8)
set(DISPLAY_KBD_MARGIN_T  6)
set(DISPLAY_FONT_SIZE     7)

configure_file(
    ${CMAKE_SOURCE_DIR}/templates/display_config.hpp.in
    ${CMAKE_BINARY_DIR}/src/overboard/hal/display_config.hpp
    @ONLY
)

# Embed keyboard.json as C++ resource
file(READ ${CMAKE_SOURCE_DIR}/data/configs/picocalc/keyboard.json JSON_CONTENT)
configure_file(
    ${CMAKE_SOURCE_DIR}/templates/embedded_json.hpp.in
    ${CMAKE_BINARY_DIR}/src/overboard/resources/embedded_json.hpp
    @ONLY
)

# SDL-specific source files
set(HARDWARE_SOURCES
    src/overboard/hal/sdl/app.cpp
    src/overboard/hal/sdl/display.cpp
    src/overboard/hal/sdl/input.cpp
    src/overboard/hal/sdl/input_key_mapping.cpp
    src/overboard/hal/sdl/keymap.cpp
    src/overboard/hal/sdl/os_utils.cpp
    src/overboard/hal/app_common.cpp
    src/overboard/hal/file_settings_store.cpp
    src/overboard/hal/host_info.cpp
    src/overboard/hal/sdl/system_info.cpp
    src/overboard/hal/settings_store_factory.cpp
    src/overboard/hal/settings_tree.cpp
)
