/**
 * @file    main.cpp
 * @author  Marvin Smith
 * @date    2025-10-19
 * @brief   Main entry point for the calculator application.
 */

// C++ Standard Libraries
#include <iostream>

// Project Libraries
#include <overboard/core/config.hpp>
#include <overboard/hal/app_factory.hpp>
#include <overboard/hal/config/target.hpp>
#include <overboard/log/stdout_logger.hpp>
#include <overboard/version.hpp>

using namespace ovb;

int main(int argc, char* argv[]) {
    auto config_opt = core::Config::parse(argc, argv);
    if (!config_opt) {
        return 1;
    }

    const auto& config = config_opt.value();

    if (config.help_requested()) {
        core::Config::print_usage(config.program_name());
        return 0;
    }

    log::Stdout_Logger::initialize(config.log_level());

    // Log version information
    LOG_INFO("kbd_calc v", PROJECT_VERSION, "\n  build: ", BUILD_DATE, "\n  git: ", GIT_HASH, GIT_DIRTY ? " (dirty)" : "");

    auto layout = hal::config::create_layout(config.layout_path());

    // Create platform-specific application via factory
    auto app = hal::App_Factory::create( layout,
                                         config.layout_path() );

    if (!app) {
        std::cerr << "Failed to create application\n";
        return 1;
    }

    // Run the main event loop
    app->run();

    return 0;
}
