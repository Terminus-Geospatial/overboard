cmake_minimum_required(VERSION 4.0)

# Load version configuration
include(cmake/scripts/version.cmake)


#  Configure the CMake Project
project( calc_simulator
         LANGUAGES C CXX ASM
         VERSION ${PROJECT_VERSION}
         DESCRIPTION "Calculator with keyboard-driven interface"
         HOMEPAGE_URL https://github.com/marvins/kbd_calc)

# Initialize Pico SDK for embedded targets (after project(), before other setup)
if(TARGET_DEVICE STREQUAL "PICOCALC")
    if(NOT DEFINED ENV{PICO_SDK_PATH})
        message(FATAL_ERROR "PICO_SDK_PATH environment variable not set for embedded target")
    endif()
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
    # Set platform/board BEFORE pico_sdk_init()
    set(PICO_PLATFORM rp2350 CACHE STRING "Pico platform" FORCE)
    set(PICO_BOARD pico2 CACHE STRING "Pico board" FORCE)
    # Point to pre-built picotool so the SDK can generate UF2 output.
    # Set PICOTOOL_DIR env var to the directory containing picotoolConfig.cmake.
    if(DEFINED ENV{PICOTOOL_DIR})
        set(picotool_DIR $ENV{PICOTOOL_DIR} CACHE PATH "Path to picotool cmake config" FORCE)
    endif()
    # Enable RTTI — dynamic_cast is used throughout the AST node hierarchy
    set(PICO_CXX_ENABLE_RTTI 1 CACHE BOOL "Enable C++ RTTI" FORCE)
    include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
    pico_sdk_init()
endif()

#  C++ Standard Configuration
set( CMAKE_CXX_STANDARD 23 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

#  Options Configuration
option( SIMULATOR            "Build simulator target (requires SDL2)"    ON )
option( BUILD_TESTS          "Build unit tests"                          ON )
option( SHOW_KEY_MAPPING     "Show interactive key mapping panel"        ON )
option( DEBUG_LAYOUT         "Draw debug borders around LCD regions"     OFF )
option( USE_ASAN             "Enable AddressSanitizer for memory debugging" OFF )

# Target device selection: SDL, PICOSDL, PICOCALC, ZERO, or WASM
if(NOT DEFINED TARGET_DEVICE OR TARGET_DEVICE STREQUAL "")
    set(TARGET_DEVICE "SDL" CACHE STRING "Target device (SDL, PICOSDL, PICOCALC, ZERO, WASM)" FORCE)
else()
    set(TARGET_DEVICE ${TARGET_DEVICE} CACHE STRING "Target device (SDL, PICOSDL, PICOCALC, ZERO, WASM)" FORCE)
endif()
set_property(CACHE TARGET_DEVICE PROPERTY STRINGS SDL PICOSDL PICOCALC ZERO WASM)

# Set keyboard UI defaults based on target
if(TARGET_DEVICE STREQUAL "SDL")
    set(SHOW_KEY_MAPPING ON CACHE BOOL "Show interactive key mapping panel" FORCE)
elseif(TARGET_DEVICE STREQUAL "PICOSDL")
    set(SHOW_KEY_MAPPING ON CACHE BOOL "Show interactive key mapping panel" FORCE)
elseif(TARGET_DEVICE STREQUAL "PICOCALC")
    set(SHOW_KEY_MAPPING OFF CACHE BOOL "Show interactive key mapping panel" FORCE)
elseif(TARGET_DEVICE STREQUAL "ZERO")
    set(SHOW_KEY_MAPPING ON CACHE BOOL "Show interactive key mapping panel" FORCE)
elseif(TARGET_DEVICE STREQUAL "WASM")
    set(SHOW_KEY_MAPPING ON CACHE BOOL "Show interactive key mapping panel" FORCE)
    set(SIMULATOR OFF CACHE BOOL "Build simulator target" FORCE)
    set(BUILD_TESTS OFF CACHE BOOL "Build unit tests" FORCE)
endif()

# ── Shared Dependencies ───────────────────────────────────────────────────────────
include(cmake/dependencies.cmake)

# ── Hardware Configuration ────────────────────────────────────────────────────────
if(TARGET_DEVICE STREQUAL "SDL")
    include(data/hardware/sdl/CMakeLists.txt)
elseif(TARGET_DEVICE STREQUAL "PICOSDL")
    include(data/hardware/picosdl/CMakeLists.txt)
elseif(TARGET_DEVICE STREQUAL "PICOCALC")
    include(data/hardware/picocalc/CMakeLists.txt)
elseif(TARGET_DEVICE STREQUAL "ZERO")
    include(data/hardware/pi_zero/CMakeLists.txt)
elseif(TARGET_DEVICE STREQUAL "WASM")
    include(data/hardware/wasm/CMakeLists.txt)
else()
    message(FATAL_ERROR "Unknown TARGET_DEVICE: ${TARGET_DEVICE}")
endif()

# ── MK20 Macropad Support ────────────────────────────────────────────────────
# POSIX serial (termios) — enabled for SDL dev targets and Pi Zero.
# PICOCALC (bare-metal RP2350) and WASM are excluded.
if(TARGET_DEVICE STREQUAL "SDL" OR TARGET_DEVICE STREQUAL "PICOSDL" OR TARGET_DEVICE STREQUAL "ZERO")
    list(APPEND HARDWARE_SOURCES
        src/overboard/hal/mk20/factory.cpp
        src/overboard/hal/mk20/input.cpp
        src/overboard/hal/mk20/keymap.cpp
        src/overboard/mpad/device.cpp
        src/overboard/mpad/key_icon.cpp
        src/overboard/mpad/protocol.cpp
        src/overboard/mpad/serial_port.cpp
        src/overboard/mpad/theme.cpp
    )
    list(APPEND HARDWARE_COMPILE_DEFINITIONS OVB_HAS_MK20=1)
else()
    list(APPEND HARDWARE_COMPILE_DEFINITIONS OVB_HAS_MK20=0)
endif()

# ── Common Sources ────────────────────────────────────────────────────────────────
set(CORE_SOURCES
    src/overboard/core/config.cpp
    src/overboard/core/keymap.cpp
    src/overboard/core/action_code.cpp
    src/overboard/core/input_key.cpp
    src/overboard/core/keyboard_layout.cpp
    src/overboard/core/layer_manager.cpp
    src/overboard/core/location_provider.cpp
    src/overboard/core/settings_manager.cpp
    src/overboard/core/solar_calc.cpp
    src/overboard/font/font_metrics.cpp
    src/overboard/font/font_selector.cpp
    src/overboard/io/file_utils.cpp
    src/overboard/io/keyboard_config.cpp
    src/overboard/math/ast/binary_op_node.cpp
    src/overboard/math/ast/function_node.cpp
    src/overboard/math/ast/matrix_node.cpp
    src/overboard/math/ast/number_node.cpp
    src/overboard/math/ast/unary_op_node.cpp
    src/overboard/math/calc_engine.cpp
    src/overboard/math/expression.cpp
    src/overboard/math/layout/engine.cpp
    src/overboard/math/parser.cpp
)

# ── GUI Sources ───────────────────────────────────────────────────────────────────
set(GUI_SOURCES
    src/overboard/gui/app_menu.cpp
    src/overboard/gui/app_view.cpp
    src/overboard/gui/dimension_picker_popup.cpp
    src/overboard/gui/footer_bar.cpp
    src/overboard/gui/function_menu_popup.cpp
    src/overboard/gui/header_bar.cpp
    src/overboard/gui/key_mapping_info.cpp
    src/overboard/gui/keyboard_display.cpp
    src/overboard/gui/lcd_section.cpp
    src/overboard/gui/lv_font_superscript_regular.c
    src/overboard/gui/lv_font_superscript_bold.c
    src/overboard/gui/math_canvas.cpp
    src/overboard/gui/panel_manager.cpp
)

# ── App Sources ────────────────────────────────────────────────────────────────────
set(APP_SOURCES
    src/overboard/apps/calculator/calculator_app.cpp
    src/overboard/apps/settings/settings_page.cpp
    src/overboard/apps/status/status_page.cpp
    src/overboard/apps/status/popups/connections_popup.cpp
    src/overboard/apps/status/widgets/analog_clock.cpp
    src/overboard/apps/status/widgets/digital_clock.cpp
    src/overboard/apps/status/widgets/solar_info.cpp
)

# ── Build Main Application ─────────────────────────────────────────────────────────
if(TARGET_DEVICE STREQUAL "SDL" OR TARGET_DEVICE STREQUAL "PICOSDL")
    set(APP_TARGET calc_sim)
    set(APP_LIBRARIES ${HARDWARE_LIBRARIES} lvgl)
elseif(TARGET_DEVICE STREQUAL "PICOCALC")
    set(APP_TARGET calc_firmware)
    set(APP_LIBRARIES ${HARDWARE_LIBRARIES} lvgl)
elseif(TARGET_DEVICE STREQUAL "ZERO")
    set(APP_TARGET calc_app)
    set(APP_LIBRARIES lvgl ${HARDWARE_LIBRARIES})
elseif(TARGET_DEVICE STREQUAL "WASM")
    set(APP_TARGET calc_sim)
    set(APP_LIBRARIES ${HARDWARE_LIBRARIES} lvgl)
endif()

#  Build the main executable
add_executable(${APP_TARGET}
    src/main.cpp
    src/overboard/hal/app_factory.cpp
    ${HARDWARE_SOURCES}
    src/overboard/log/stdout_logger.cpp
    src/overboard/log/log_level.cpp
    ${CORE_SOURCES}
    ${GUI_SOURCES}
    ${APP_SOURCES}
)

#   Setup Include Paths
target_include_directories( ${APP_TARGET} PRIVATE src )
target_include_directories( ${APP_TARGET} PRIVATE ${CMAKE_BINARY_DIR}/src )
target_include_directories( ${APP_TARGET} SYSTEM PRIVATE thirdparty/lvgl )
target_include_directories( ${APP_TARGET} SYSTEM PRIVATE thirdparty/stb )
target_include_directories( ${APP_TARGET} SYSTEM PRIVATE thirdparty/json/include )
target_include_directories( ${APP_TARGET} SYSTEM PRIVATE ${HARDWARE_INCLUDE_DIRS} )

#-------------------------------#
#-       Link Dependencies     -#
#-------------------------------#
target_link_libraries(${APP_TARGET} PRIVATE
    ${APP_LIBRARIES}
    project_warnings
    nlohmann_json
    tomlplusplus
)

if(SHOW_KEY_MAPPING)
    target_compile_definitions(${APP_TARGET} PRIVATE SHOW_KEY_MAPPING=1)
else()
    target_compile_definitions(${APP_TARGET} PRIVATE SHOW_KEY_MAPPING=0)
endif()

target_compile_definitions(${APP_TARGET} PRIVATE ${HARDWARE_COMPILE_DEFINITIONS})

if(DEBUG_LAYOUT)
    target_compile_definitions(${APP_TARGET} PRIVATE OVB_DEBUG_LAYOUT=1)
endif()

if(USE_ASAN)
    target_compile_options(${APP_TARGET} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(${APP_TARGET} PRIVATE -fsanitize=address)
endif()

if(TARGET_DEVICE STREQUAL "WASM")
    set_target_properties(${APP_TARGET} PROPERTIES
        SUFFIX "${WASM_TARGET_SUFFIX}"
    )
    target_compile_options(${APP_TARGET} PRIVATE ${WASM_COMPILE_OPTIONS})
    target_link_options(${APP_TARGET} PRIVATE ${WASM_LINK_FLAGS})
endif()

if(TARGET_DEVICE STREQUAL "PICOCALC")
    set_target_properties(${APP_TARGET} PROPERTIES SUFFIX ".elf")
    pico_add_extra_outputs(${APP_TARGET})
endif()

# ── Keylogger Utility ────────────────────────────────────────────────────────────────
# Enabled per-target via hardware-specific CMakeLists.txt
if(BUILD_KEYLOGGER)
    set(KEYLOGGER_SOURCES
        src/overboard/core/input_key.cpp
        src/overboard/log/log_level.cpp
        src/overboard/log/stdout_logger.cpp
        src/overboard/tools/keylogger.cpp
    )

    add_executable(keylogger ${KEYLOGGER_SOURCES})
    target_include_directories(keylogger PRIVATE src)

    # Add target-specific sources and defines
    if(KEYLOGGER_USE_SDL)
        target_sources(keylogger PRIVATE
            src/overboard/hal/sdl/input.cpp
            src/overboard/hal/sdl/input_key_mapping.cpp
            src/overboard/hal/sdl/keymap.cpp
        )
        target_include_directories(keylogger SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS})
        target_link_libraries(keylogger PRIVATE ${SDL2_LIBRARIES})
        target_compile_definitions(keylogger PRIVATE TARGET_SDL=1)
    else()
        # Pi Zero / evdev version
        target_sources(keylogger PRIVATE src/overboard/hal/pi_zero/input_linux.cpp)
        target_compile_definitions(keylogger PRIVATE TARGET_ZERO=1)
    endif()

    target_link_libraries(keylogger PRIVATE project_warnings)
endif()

# ── Test Math Rendering Tool (SDL-only) ───────────────────────────────────────────────
if(TARGET_DEVICE STREQUAL "SDL" OR TARGET_DEVICE STREQUAL "PICOSDL")
    add_executable(test_math_render
        src/overboard/tools/test_math_render/test_math_render.cpp
        src/overboard/tools/test_math_render/config.cpp
        src/overboard/font/font_metrics.cpp
        src/overboard/math/ast/binary_op_node.cpp
        src/overboard/math/ast/function_node.cpp
        src/overboard/math/ast/matrix_node.cpp
        src/overboard/math/ast/number_node.cpp
        src/overboard/math/ast/unary_op_node.cpp
        src/overboard/math/parser.cpp
        src/overboard/math/layout/engine.cpp
        src/overboard/log/stdout_logger.cpp
        src/overboard/log/log_level.cpp
    )
    target_include_directories(test_math_render PRIVATE src)
    target_include_directories(test_math_render PRIVATE thirdparty/stb)
    target_include_directories(test_math_render SYSTEM PRIVATE ${PNG_INCLUDE_DIRS})
    target_include_directories(test_math_render SYSTEM PRIVATE thirdparty/lvgl)
    target_link_libraries(test_math_render PRIVATE PNG::PNG lvgl)
    target_compile_options(test_math_render PRIVATE -Wno-old-style-cast -Wno-sign-conversion)

    # Keyboard layout renderer tool
    add_executable(render_layout
        src/overboard/core/action_code.cpp
        src/overboard/core/input_key.cpp
        src/overboard/core/keyboard_layout.cpp
        src/overboard/core/keymap.cpp
        src/overboard/io/file_utils.cpp
        src/overboard/io/keyboard_config.cpp
        src/overboard/log/log_level.cpp
        src/overboard/log/stdout_logger.cpp
        src/overboard/tools/ovt_layout_util/config.cpp
        src/overboard/tools/ovt_layout_util/main.cpp
    )
    target_include_directories(render_layout PRIVATE src)
    target_include_directories(render_layout PRIVATE ${CMAKE_BINARY_DIR}/src)
    target_include_directories(render_layout SYSTEM PRIVATE thirdparty/json/include ${PNG_INCLUDE_DIRS})
    target_link_libraries(render_layout PRIVATE nlohmann_json PNG::PNG project_warnings)

    # MK20 key-icon renderer (LVGL glyph -> PNG)
    add_executable(ovt_icon_render
        src/overboard/log/log_level.cpp
        src/overboard/log/stdout_logger.cpp
        src/overboard/mpad/key_icon.cpp
        src/overboard/tools/ovt_icon_render/config.cpp
        src/overboard/tools/ovt_icon_render/main.cpp
    )
    target_include_directories(ovt_icon_render PRIVATE src)
    target_include_directories(ovt_icon_render SYSTEM PRIVATE thirdparty/stb)
    target_include_directories(ovt_icon_render SYSTEM PRIVATE thirdparty/lvgl)
    target_link_libraries(ovt_icon_render PRIVATE lvgl)
    target_compile_options(ovt_icon_render PRIVATE -Wno-old-style-cast -Wno-sign-conversion)
endif()

# ── Packaging (Pi Zero only) ──────────────────────────────────────────────────────
if(TARGET_DEVICE STREQUAL "ZERO")
    include(cmake/packaging.cmake)
endif()

# ── Tests (shared) ────────────────────────────────────────────────────────────────
# Tests only for simulator targets (gtest doesn't build for embedded ARM or Pi Zero)
if( BUILD_TESTS AND NOT (TARGET_DEVICE STREQUAL "PICOCALC" OR TARGET_DEVICE STREQUAL "ZERO") )
    add_executable(calc_tests
        test/test_main.cpp
        test/math/ast/test_ast.cpp
        test/math/test_parser.cpp
        test/math/layout/test_render.cpp
        test/math/layout/test_layout_engine.cpp
        test/math/test_calc_engine.cpp
        test/math/test_expression.cpp
        test/math/test_matrix_ops.cpp
        test/math/test_trig_functions.cpp
        test/core/test_layout.cpp
        test/core/test_action_code.cpp
        test/core/test_input_key.cpp
        test/core/test_location_provider.cpp
        test/core/test_settings_manager.cpp
        test/font/test_font_metrics.cpp
        test/hal/test_settings_tree.cpp
        src/overboard/core/keymap.cpp
        src/overboard/core/settings_manager.cpp
        src/overboard/core/location_provider.cpp
        src/overboard/core/solar_calc.cpp
        src/overboard/hal/settings_tree.cpp
        src/overboard/core/action_code.cpp
        src/overboard/core/input_key.cpp
        src/overboard/core/layer_manager.cpp
        src/overboard/font/font_metrics.cpp
        src/overboard/math/ast/binary_op_node.cpp
        src/overboard/math/ast/function_node.cpp
        src/overboard/math/ast/matrix_node.cpp
        src/overboard/math/ast/number_node.cpp
        src/overboard/math/ast/unary_op_node.cpp
        src/overboard/math/expression.cpp
        src/overboard/math/calc_engine.cpp
        src/overboard/math/parser.cpp
        src/overboard/math/layout/engine.cpp
        src/overboard/log/stdout_logger.cpp
        src/overboard/log/log_level.cpp
    )

    target_include_directories(calc_tests PRIVATE src test)
    target_include_directories(calc_tests PRIVATE ${CMAKE_BINARY_DIR}/src)
    target_include_directories(calc_tests SYSTEM PRIVATE thirdparty/lvgl)
    target_include_directories(calc_tests SYSTEM PRIVATE thirdparty/json/include)
    if(SDL2_INCLUDE_DIRS)
        target_include_directories(calc_tests SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS})
    elseif(TARGET SDL2::SDL2)
        get_target_property(_sdl2_inc SDL2::SDL2 INTERFACE_INCLUDE_DIRECTORIES)
        if(_sdl2_inc)
            target_include_directories(calc_tests SYSTEM PRIVATE ${_sdl2_inc})
        endif()
    endif()
    target_link_libraries(calc_tests PRIVATE GTest::gtest project_warnings lvgl nlohmann_json tomlplusplus)
    include(GoogleTest)
    gtest_discover_tests(calc_tests)
endif()
