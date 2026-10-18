/**
 * @file    config.cpp
 * @author  Marvin Smith
 * @date    5/25/2026
 *
 * @brief Configuration for the test_math_render application
 */
#include <overboard/tools/test_math_render/config.hpp>

// C++ Standard Library
#include <deque>
#include <iostream>
#include <sstream>
#include <string>

// Project Libraries
#include <overboard/log/stdout_logger.hpp>

namespace ovb {

/****************************************/
/*           Print Usage Information    */
/****************************************/
void Config::usage() const
{
    std::cout << "Usage: " << m_app_name << " [font.ttf] <expression> -o <output.png> [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Required:" << std::endl;
    std::cout << "  expression         Math expression to render (e.g., \"sqrt(x)\", \"2^2\")" << std::endl;
    std::cout << "  -o <output.png>   Output PNG file path" << std::endl;
    std::cout << std::endl;
    std::cout << "Optional:" << std::endl;
    std::cout << "  font.ttf          TTF font file (defaults to 5x7 bitmap font)" << std::endl;
    std::cout << "  --debug           Draw debug boxes around layout elements" << std::endl;
    std::cout << "  --scale <factor>  Scale factor for font size (default: 1.0)" << std::endl;
    std::cout << "  --width <pixels>  Canvas width in pixels (default: 60)" << std::endl;
    std::cout << "  --height <pixels> Canvas height in pixels (default: 60)" << std::endl;
    std::cout << "  --log-level <level> Log level: debug, info, warn, error (default: info)" << std::endl;
    std::cout << "  -h, --help        Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << m_app_name << " \"sqrt(x)\" -o output/sqrt.png" << std::endl;
    std::cout << "  " << m_app_name << " DejaVuSans.ttf \"2^2\" -o output/power_2.png --debug" << std::endl;
    std::cout << "  " << m_app_name << " \"pi\" -o output/pi.png --scale 2 --width 100 --height 100" << std::endl;
}

/****************************************/
/*         Configure Logging            */
/****************************************/
void Config::configure_logging()
{
    // Create stdout logger with configured severity
    log::Stdout_Logger logger(m_log_severity);

    logger.debug( "Logging Configured\n", to_string() );
}

/****************************************/
/*         Log Configuration            */
/****************************************/
std::string Config::to_string() const
{
    std::stringstream ss;
    ss << "Configuration:" << std::endl;
    ss << "  Application: " << m_app_name << std::endl;
    ss << "  Expression: " << m_expression << std::endl;
    ss << "  Output Path: " << m_output_path << std::endl;
    ss << "  Font Path: " << (m_font_path.empty() ? "5x7 (built-in)" : m_font_path.string()) << std::endl;
    ss << "  Use 5x7: " << (m_use_5x7 ? "true" : "false") << std::endl;
    ss << "  Debug Boxes: " << (m_debug_boxes ? "true" : "false") << std::endl;
    ss << "  Scale: " << m_scale << std::endl;
    ss << "  Width: " << m_width << std::endl;
    ss << "  Height: " << m_height << std::endl;
    ss << "  Log Level: " << log::to_string(m_log_severity) << std::endl;

    return ss.str();
}

/****************************************/
/*      Parse Command-Line Arguments    */
/****************************************/
Config Config::parse_command_line( int argc,
                                   char* argv[],
                                   [[maybe_unused]] char** envp )
{
    Config config;
    config.m_app_name = argv[0];
    config.m_log_severity = log::Log_Level::Info;  // Default

    // Parse arguments
    std::deque<std::string> args(argv + 1, argv + argc);
    while (!args.empty())
    {
        const std::string& arg = args.front();
        args.pop_front();

        // Help / Usage
        if (arg == "--help" || arg == "-h")
        {
            config.usage();
            std::exit(0);
        }

        // Check for output path
        else if (arg == "--output" || arg == "-o")
        {
            if (args.empty())
            {
                std::cerr << "Error: --output requires an argument" << std::endl;
                std::exit(1);
            }
            config.m_output_path = args.front();
            args.pop_front();
        }

        // Debug boxes
        else if (arg == "--debug")
        {
            config.m_debug_boxes = true;
        }

        // Scale factor
        else if (arg == "--scale")
        {
            if (args.empty())
            {
                std::cerr << "Error: --scale requires an argument" << std::endl;
                std::exit(1);
            }
            config.m_scale = std::stof(args.front());
            args.pop_front();
        }

        // Canvas width
        else if (arg == "--width")
        {
            if (args.empty())
            {
                std::cerr << "Error: --width requires an argument" << std::endl;
                std::exit(1);
            }
            config.m_width = std::stoi(args.front());
            args.pop_front();
        }

        // Canvas height
        else if (arg == "--height")
        {
            if (args.empty())
            {
                std::cerr << "Error: --height requires an argument" << std::endl;
                std::exit(1);
            }
            config.m_height = std::stoi(args.front());
            args.pop_front();
        }

        // Log level
        else if (arg == "--log-level")
        {
            if (args.empty())
            {
                std::cerr << "Error: --log-level requires an argument" << std::endl;
                std::exit(1);
            }
            std::string level_str = args.front();
            args.pop_front();

            if (level_str == "debug")
                config.m_log_severity = log::Log_Level::Debug;
            else if (level_str == "info")
                config.m_log_severity = log::Log_Level::Info;
            else if (level_str == "warn")
                config.m_log_severity = log::Log_Level::Warn;
            else if (level_str == "error")
                config.m_log_severity = log::Log_Level::Error;
            else
            {
                std::cerr << "Error: Invalid log level: " << level_str << std::endl;
                std::exit(1);
            }
        }

        // Positional arguments
        else
        {
            // Check if it's a TTF font file
            if (arg.find(".ttf") != std::string::npos || arg.find(".ttc") != std::string::npos)
            {
                config.m_font_path = arg;
                config.m_use_5x7 = false;
            }
            // Otherwise, treat as expression
            else if (config.m_expression.empty())
            {
                config.m_expression = arg;
            }
            else
            {
                std::cerr << "Error: Unknown argument or duplicate expression: " << arg << std::endl;
                std::exit(1);
            }
        }
    }

    // Validate required arguments
    if (config.m_expression.empty())
    {
        std::cerr << "Error: Expression is required" << std::endl;
        config.usage();
        std::exit(1);
    }

    if (config.m_output_path.empty())
    {
        std::cerr << "Error: -o <output.png> is required" << std::endl;
        config.usage();
        std::exit(1);
    }

    return config;
}

} // End of ovb namespace