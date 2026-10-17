# SDL Simulator hardware configuration
# This file contains SDL-specific build settings for the simulator target.
# It is included by the main CMakeLists.txt when TARGET_DEVICE=SDL.

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

# Keyboard layout config — MK20 macropad by default; override for other
# layouts, e.g. -DOVB_KEYBOARD_CONFIG=data/configs/th33/
set(OVB_KEYBOARD_CONFIG "data/configs/mk20/" CACHE STRING
    "Keyboard config directory (relative to repo root)")

# Set up SDL-specific compile definitions
set(HARDWARE_COMPILE_DEFINITIONS
    TARGET_SDL=1
    SIMULATOR=1
    SHOW_KEYBOARD_UI=1
    DEFAULT_CONFIG_PATH="${OVB_KEYBOARD_CONFIG}"
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

# Display dimensions for the SDL simulator
# Main window: LCD (480x500) + Key Mapping Info (480x300) = 480x800
# Keyboard window: separate window (480x300)
# Note: KBD_HEIGHT=300 reserves space for Key_Mapping_Info in main window.
#       App_View won't create Keyboard_Display there (SDL_App does it separately).
set(DISPLAY_FULL_WIDTH  480)
set(DISPLAY_FULL_HEIGHT 800)
set(DISPLAY_KBD_WIDTH   480)
set(DISPLAY_KBD_HEIGHT  300)
set(DISPLAY_KBD_WIN_HEIGHT 300)
set(DISPLAY_PREVIEW_MAX_WIDTH  464)  # LCD_WIDTH - 16 (padding)
set(DISPLAY_PREVIEW_MAX_HEIGHT 80)   # Standard entry height

set(DISPLAY_KEY_PAD       2)
set(DISPLAY_KBD_HEADER_H 20)
set(DISPLAY_KBD_MARGIN_L  8)
set(DISPLAY_KBD_MARGIN_T  6)
set(DISPLAY_FONT_SIZE     10)

configure_file(
    ${CMAKE_SOURCE_DIR}/templates/display_config.hpp.in
    ${CMAKE_BINARY_DIR}/src/overboard/hal/display_config.hpp
    @ONLY
)

# Embed keyboard.json as C++ resource
file(READ ${CMAKE_SOURCE_DIR}/${OVB_KEYBOARD_CONFIG}keyboard.json JSON_CONTENT)
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
    src/overboard/hal/sdl/keyboard_window.cpp
    src/overboard/hal/sdl/os_utils.cpp
    src/overboard/hal/sdl/settings_store.cpp
    src/overboard/hal/sdl/system_info.cpp
    src/overboard/hal/settings_store_factory.cpp
    src/overboard/hal/settings_tree.cpp
)

# Enable keylogger for SDL (uses SDL input)
set(BUILD_KEYLOGGER ON)
set(KEYLOGGER_USE_SDL ON)
