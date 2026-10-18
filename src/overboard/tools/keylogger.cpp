/**
 * @file    keylogger.cpp
 * @author  Marvin Smith
 * @date    2026-05-22
 *
 * @brief   Keylogger utility for mapping physical keys
 *
 * Uses HAL input interface (SDL for simulator, evdev for Pi Zero).
 * Useful for determining key mappings on real hardware.
 */

// C++ Standard Libraries
#include <csignal>
#include <cstdio>
#include <ctime>
#include <memory>
#include <string>
#include <vector>

// Third-Party Libraries
#if defined(TARGET_SDL)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wold-style-cast"
    #include <SDL2/SDL.h>
    #pragma GCC diagnostic pop
#endif

// Project Libraries
#include <overboard/core/input_key.hpp>
#include <overboard/hal/i_input.hpp>
#include <overboard/log/stdout_logger.hpp>

#if defined(TARGET_SDL)
    #include <overboard/hal/sdl/input.hpp>
#else
    #include <overboard/hal/pi_zero/input_linux.hpp>
#endif

// Global quit flag for signal handling
static volatile bool g_quit = false;

static void signal_handler(int) {
    g_quit = true;
}

/**
 * @brief Main keylogger loop
 */
int main(int /*argc*/, char* /*argv*/[]) {
    ovb::log::Stdout_Logger::initialize(ovb::log::Log_Level::Info);

    // Set up signal handlers for clean exit
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    LOG_INFO("=== Keylogger Starting ===");

#if defined(TARGET_SDL)
    LOG_INFO("Using SDL input");
    LOG_INFO("Press keys to see SDL scancodes");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    // Create a small window to receive keyboard events
    SDL_Window* window = SDL_CreateWindow(
        "Keylogger - Press keys to log",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        400, 200,
        SDL_WINDOW_SHOWN);

    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    auto input = std::make_unique<ovb::hal::sdl::SDL_Input>();
#else
    LOG_INFO("Using Linux input (evdev)");
    LOG_INFO("Reading from /dev/input/event* devices");

    // Auto-detect keyboard devices
    std::vector<std::string> device_paths;
    for (int i = 0; i < 10; ++i) {
        device_paths.push_back("/dev/input/event" + std::to_string(i));
    }

    auto input = std::make_unique<ovb::hal::pi_zero::Linux_Input>(device_paths);
#endif

    LOG_INFO("Press Ctrl+C to exit");
    LOG_INFO("===========================");

    bool running = true;
    while (running && !g_quit && !input->should_quit()) {
        // Pump events
        input->pump();

        // Poll for key events
        ovb::hal::Key_Event event;
        while (input->poll(event)) {
            std::string msg;

            if (event.kind == ovb::hal::Key_Event_Kind::Action) {
                std::string type_str = (event.type == ovb::hal::Key_Event_Type::Press) ? "KEYDOWN" : "KEYUP";
                auto input_key = static_cast<ovb::core::Input_Key>(event.key_index);
                msg = type_str + ": " + ovb::core::input_key_to_string(input_key);
            } else {
                // Text event
                msg = "TEXT: codepoint=" + std::to_string(event.codepoint);
            }

            LOG_INFO(msg);
        }

#if defined(TARGET_SDL)
        // Handle SDL window events
        SDL_Event sdl_event;
        while (SDL_PollEvent(&sdl_event)) {
            if (sdl_event.type == SDL_QUIT) {
                running = false;
            }
        }
        SDL_Delay(10);
#else
        // Small delay for Pi Zero
        struct timespec ts = {0, 10 * 1000 * 1000};  // 10ms
        nanosleep(&ts, nullptr);
#endif
    }

    LOG_INFO("=== Keylogger Exiting ===");

#if defined(TARGET_SDL)
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif

    return 0;
}
