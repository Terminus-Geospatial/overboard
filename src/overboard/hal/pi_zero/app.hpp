/**
 * @file    app.hpp
 * @author  Marvin Smith
 * @date    2026-06-05
 *
 * @brief   Pi Zero DRM application implementation
 *
 * Implements I_App for the Pi Zero target using direct DRM/KMS display
 * and Linux input events. Runs on the console without X11.
 */
#pragma once

// C++ Standard Libraries
#include <array>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>

// Project Libraries
#include <overboard/core/input_key.hpp>
#include <overboard/core/keyboard_layout.hpp>
#include <overboard/core/keymap.hpp>
#include <overboard/core/layer_manager.hpp>
#include <overboard/gui/app_view.hpp>
#include <overboard/gui/i_keymap_view.hpp>
#include <overboard/gui/key_mapping_info.hpp>
#include <overboard/hal/display_config.hpp>
#include <overboard/hal/i_app.hpp>
#include <overboard/hal/i_system_info.hpp>
#include <overboard/hal/pi_zero/display_drm.hpp>
#include <overboard/hal/pi_zero/input_linux.hpp>
#include <overboard/hal/pi_zero/system_info.hpp>
#include <overboard/io/keyboard_config.hpp>
#include <overboard/math/calc_engine.hpp>

#if OVB_HAS_MK20
#include <overboard/hal/mk20/input.hpp>
#include <overboard/hal/mk20/keymap.hpp>
#include <overboard/mpad/device.hpp>
#endif

namespace ovb::hal::pi_zero {

/**
 * @brief Pi Zero DRM application implementation
 *
 * Manages the Linux DRM lifecycle for direct HDMI output. Creates the display,
 * handles the main event loop via Linux input, and routes key events to the
 * calculator engine.
 */
class PiZero_App : public I_App {

    public:

        /// @brief Display width
        static constexpr int DISPLAY_WIDTH = ovb::hal::FULL_WIDTH;

        /// @brief Display height
        static constexpr int DISPLAY_HEIGHT = ovb::hal::FULL_HEIGHT;

        /**
         * @brief Factory method to create and initialize Pi Zero application
         * @param layout Keyboard layout for the target device
         * @param layout_path Path to keyboard.json file
         * @return Unique pointer to initialized PiZero_App, or nullptr on failure
         */
        static std::unique_ptr<PiZero_App> create(const core::Grid_Layout& layout,
                                                  const std::filesystem::path& layout_path);

        /**
         * Destructor
         */
        ~PiZero_App() override;

        /**
         * @brief Initialize Pi Zero application
         * @return true if initialization succeeded, false otherwise
         */
        bool init() override;

        /**
         * @brief Run the main event loop
         */
        void run() override;

        /**
         * @brief Check if the application should quit
         * @return true if application should quit, false otherwise
         */
        bool       should_quit() const override;
        I_Display& get_display()  override;

        /**
         * @brief Get the input interface
         * @return Reference to the Linux input handler
         */
        I_Input& get_input() override;

        /**
         * @brief Handle a key press event
         * @param key_index Raw Input_Key enum value or layout index
         */
        void handle_key(int key_index);

        /**
         * @brief Dispatch an action by resolved layout index
         * @param layout_idx     Resolved key index in the layout/layer
         * @param raw_input_key  Original Input_Key enum value, or -1 (serial/click)
         */
        void handle_layout_key(int layout_idx, int raw_input_key);

    private:

        /**
         * @brief Private constructor - use create() factory method
         * @param layout Keyboard layout reference
         */
        explicit PiZero_App(const core::Grid_Layout& layout);

        /// @brief Callback function for key clicks
        static void on_key_clicked(int key_index, void* user_data);

        /// @brief Reference to the keyboard layout
        const core::Grid_Layout&          m_layout;

        /// @brief Path to the layout file
        std::filesystem::path             m_layout_path;

        /// @brief Flag indicating if the application has been initialized
        bool                              m_initialized = false;
        std::atomic<bool>                 m_should_quit = false;

        /// @brief Keymap for the calculator
        core::Keymap m_keymap;

        /// @brief Layer manager for the calculator
        core::Layer_Manager m_layers;

        /// @brief Per-panel MK20 screen definitions from keyboard.json
        core::Screen_Set m_screens;

        /// @brief Calculator engine for mathematical operations
        math::Calc_Engine m_engine;

        /// @brief DRM display driver
        std::unique_ptr<Display_DRM>      m_display;

        /// @brief Interactive key mapping info panel (below LCD area)
        std::unique_ptr<gui::Key_Mapping_Info> m_key_mapping_info;

        /// @brief Active keymap presenter — on-screen panel or MK20 device
        gui::I_Keymap_View*                   m_keymap_view = nullptr;

        /// @brief Declared attached device from keyboard.json (if any)
        std::optional<io::Keyboard_Config::Device_Config> m_device_config;

#if OVB_HAS_MK20
        /// @brief MK20 macropad connection (input + presenter share it)
        std::shared_ptr<mpad::Device>         m_mk20_device;

        /// @brief MK20 serial input source
        std::unique_ptr<mk20::Mk20_Input>     m_mk20_input;

        /// @brief MK20 keymap presenter (owns the device theme state)
        std::unique_ptr<mk20::Mk20_Keymap>    m_mk20_keymap;
#endif

        /// @brief LVGL application view (LCD + key mapping info panel)
        std::unique_ptr<gui::App_View>    m_view;

        /// @brief System info provider
        std::unique_ptr<PiZero_System_Info> m_system_info;

        /// @brief Linux input handler
        std::unique_ptr<Linux_Input>      m_input;

        /// @brief Maps Input_Key enum value -> layout key index (-1 if unbound)
        std::array<int, 512>              m_input_key_map {};

};

} // namespace ovb::hal::pi_zero
