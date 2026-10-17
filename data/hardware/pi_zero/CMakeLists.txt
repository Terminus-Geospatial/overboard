# Pi Zero hardware configuration
# This file contains DRM/KMS-specific build settings for Pi Zero target.
# It is included by the main CMakeLists.txt when TARGET_DEVICE=ZERO.
# Uses direct HDMI output via DRM without X11.

# Find required libraries
find_package(PNG REQUIRED)

# DRM library for direct display access
find_library(DRM_LIBRARY drm REQUIRED)
find_library(GBM_LIBRARY gbm REQUIRED)

# Find DRM include directory (drm/drm.h lives under /usr/include on Debian)
find_path(DRM_INCLUDE_DIR drm/drm.h
    PATHS
        /usr/include
        /usr/local/include
)
# xf86drm.h and xf86drmMode.h live under libdrm/ on Debian/Ubuntu
find_path(LIBDRM_INCLUDE_DIR xf86drm.h
    PATHS
        /usr/include/libdrm
        /usr/local/include/libdrm
        /usr/include
        /usr/local/include
)
if(NOT LIBDRM_INCLUDE_DIR)
    message(FATAL_ERROR "libdrm headers (xf86drm.h) not found")
endif()
message(STATUS "libdrm include dir found: ${LIBDRM_INCLUDE_DIR}")
if(DRM_INCLUDE_DIR)
    message(STATUS "DRM include dir found: ${DRM_INCLUDE_DIR}")
else()
    message(FATAL_ERROR "DRM headers not found")
endif()

message(STATUS "DRM library found: ${DRM_LIBRARY}")
message(STATUS "GBM library found: ${GBM_LIBRARY}")

# Keyboard layout config — MK20 macropad by default; override for other
# layouts, e.g. -DOVB_KEYBOARD_CONFIG=data/configs/th33/
set(OVB_KEYBOARD_CONFIG "data/configs/mk20/" CACHE STRING
    "Keyboard config directory (relative to repo root)")

# Set up Pi Zero-specific compile definitions
set(HARDWARE_COMPILE_DEFINITIONS
    TARGET_ZERO=1
    SIMULATOR=0
    DEFAULT_CONFIG_PATH="${OVB_KEYBOARD_CONFIG}"
    EMBEDDED_JSON=1
    LV_USE_SDL=0
    LV_USE_LINUX_DRM=1
)

# Pi Zero-specific libraries
set(HARDWARE_LIBRARIES
    ${DRM_LIBRARY}
    ${GBM_LIBRARY}
    PNG::PNG
    pthread
)

# Pi Zero-specific include directories
# DRM_INCLUDE_DIR is e.g. /usr/include; also add /usr/include/drm so that
# xf86drm.h's bare #include <drm.h> resolves (headers live in the drm/ subdir)
set(HARDWARE_INCLUDE_DIRS
    ${PNG_INCLUDE_DIRS}
    ${DRM_INCLUDE_DIR}
    ${DRM_INCLUDE_DIR}/drm
    ${LIBDRM_INCLUDE_DIR}
)

# Display dimensions for Pi Zero (native portrait after 270° rotation)
# Physical display: 1024x600 landscape -> logical: 600x1024 portrait
# KBD_HEIGHT=0 — the MK20 macropad is the keymap presenter; the app UI
# fills the whole framebuffer.
set(DISPLAY_FULL_WIDTH  600)
set(DISPLAY_FULL_HEIGHT 1024)
set(DISPLAY_KBD_WIDTH   600)
set(DISPLAY_KBD_HEIGHT  0)
set(DISPLAY_KBD_WIN_HEIGHT 0)
set(DISPLAY_PREVIEW_MAX_WIDTH  584)
set(DISPLAY_PREVIEW_MAX_HEIGHT 120)

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

# Pi Zero-specific source files
set(HARDWARE_SOURCES
    src/overboard/hal/pi_zero/app.cpp
    src/overboard/hal/pi_zero/display_drm.cpp
    src/overboard/hal/pi_zero/input_linux.cpp
    src/overboard/hal/pi_zero/system_info.cpp
    src/overboard/hal/pi_zero/settings_store.cpp
    src/overboard/hal/settings_store_factory.cpp
    src/overboard/hal/settings_tree.cpp
)

# Enable keylogger for Pi Zero (uses evdev/Linux input)
set(BUILD_KEYLOGGER ON)
set(KEYLOGGER_USE_SDL OFF)
