/**
 * @file    config.hpp
 * @author  Marvin Smith
 * @date    2026-10-21
 *
 * @brief   Configuration for ovt_icon_render application
 */
#pragma once

// C++ Standard Libraries
#include <filesystem>
#include <optional>
#include <string>

namespace ovb::tools::icon_render {

/**
 * @brief Application configuration
 */
class Config {

    public:

        /**
         * @brief Parse command-line arguments
         * @param argc Argument count
         * @param argv Argument values
         * @return Config instance, or nullopt if parsing failed
         */
        static std::optional<Config> parse(int argc, char* argv[]);

        /**
         * @brief Print usage information
         * @param program_path Path to the program executable
         */
        static void print_usage(const std::filesystem::path& program_path);

        /**
         * @return Glyph codepoint to render (UTF-8 decoded)
         */
        inline uint32_t codepoint() const { return m_codepoint; }

        /**
         * @return Optional caption rendered below the glyph
         */
        inline const std::string& label() const { return m_label; }

        /**
         * @return Path to output PNG file
         */
        inline const std::filesystem::path& output_path() const { return m_output_path; }

        /**
         * @return Square icon edge length in pixels
         */
        inline int size() const { return m_size; }

        /**
         * @return Background color as RGB (glyph is white)
         */
        inline uint32_t bg_rgb() const { return m_bg_rgb; }

        /**
         * @return Foreground/glyph color as RGB
         */
        inline uint32_t fg_rgb() const { return m_fg_rgb; }

        /**
         * @return true if help was requested
         */
        inline bool help_requested() const { return m_help_requested; }

    private:

        /**
         * @brief Private constructor
         */
        Config() = default;

        /// @brief Glyph codepoint (from --symbol name table or --codepoint)
        uint32_t m_codepoint = 0;

        /// @brief Optional caption text rendered beneath the glyph
        std::string m_label;

        /// @brief Path to output PNG file
        std::filesystem::path m_output_path;

        /// @brief Square icon edge length (device keys are 128px)
        int m_size = 128;

        /// @brief Background RGB — dark slate, matches the shared key icon
        uint32_t m_bg_rgb = 0x2B2B3B;

        /// @brief Glyph/caption RGB
        uint32_t m_fg_rgb = 0xFFFFFF;

        /// @brief Help requested flag
        bool m_help_requested = false;
};

} // namespace ovb::tools::icon_render
