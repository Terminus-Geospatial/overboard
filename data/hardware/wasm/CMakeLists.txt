# WASM/Emscripten hardware configuration
# This file contains Emscripten-specific build settings for the browser target.
# It is included by the main CMakeLists.txt when TARGET_DEVICE=WASM.

# Emscripten provides SDL2 via -sUSE_SDL=2; no native find_package is required.
set(HARDWARE_COMPILE_DEFINITIONS
    TARGET_SDL=1
    TARGET_WASM=1
    SIMULATOR=1
    DEFAULT_CONFIG_PATH="data/configs/th33/"
    EMBEDDED_JSON=1
)

# Emscripten port libraries
set(HARDWARE_LIBRARIES
    -sUSE_SDL=2
    -sUSE_LIBPNG=1
    -sUSE_FREETYPE=1
)

set(HARDWARE_INCLUDE_DIRS "")

# Display dimensions for the WASM browser target (matching pi-zero)
set(DISPLAY_FULL_WIDTH  600)
set(DISPLAY_FULL_HEIGHT 1024)
set(DISPLAY_KBD_WIDTH   600)
set(DISPLAY_KBD_HEIGHT  400)
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
file(READ ${CMAKE_SOURCE_DIR}/data/configs/th33/keyboard.json JSON_CONTENT)
configure_file(
    ${CMAKE_SOURCE_DIR}/templates/embedded_json.hpp.in
    ${CMAKE_BINARY_DIR}/src/overboard/resources/embedded_json.hpp
    @ONLY
)

# WASM-specific source files
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

# Disable keylogger for WASM (SDL input is available but the utility is not needed)
set(BUILD_KEYLOGGER OFF)
set(KEYLOGGER_USE_SDL OFF)

# Disable tests for WASM
set(BUILD_TESTS OFF)

# Emscripten linker flags for HTML output
set(EMSCRIPTEN_LINK_FLAGS
    -sUSE_SDL=2
    -sUSE_LIBPNG=1
    -sUSE_FREETYPE=1
    -sWASM=1
    -sINITIAL_MEMORY=256MB
    -sSTACK_SIZE=2MB
    -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']
    -sEXPORTED_FUNCTIONS=['_main','_malloc','_free']
    -sNO_EXIT_RUNTIME=1
    -sASYNCIFY=0
    -sENVIRONMENT=web
    -sMODULARIZE=0
    -sSINGLE_FILE=0
    -sNO_DISABLE_EXCEPTION_CATCHING
    --shell-file ${CMAKE_SOURCE_DIR}/templates/wasm_shell.html
    --preload-file ${CMAKE_SOURCE_DIR}/data/configs/th33@/data/configs/th33
)

# Variables for the main CMakeLists.txt to apply after APP_TARGET is defined
set(WASM_TARGET_SUFFIX ".html")
set(WASM_LINK_FLAGS "${EMSCRIPTEN_LINK_FLAGS}")
set(WASM_COMPILE_OPTIONS
    -sUSE_SDL=2
    -O2
)
