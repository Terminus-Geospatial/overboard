/**
 * @file    app.hpp
 * @author  Marvin Smith
 * @date    2026-05-22
 *
 * @brief   SDL simulator application implementation
 *
 * Implements I_App for the SDL simulator target. Handles SDL initialization,
 * window creation, event loop, and cleanup. All SDL-specific logic is
 * encapsulated here to keep main() platform-agnostic.
 */
#pragma once

// C++ Standard Libraries
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

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
#include <overboard/hal/sdl/display.hpp>
#include <overboard/hal/sdl/input.hpp>
#include <overboard/hal/sdl/system_info.hpp>
#include <overboard/io/keyboard_config.hpp>
#include <overboard/math/calc_engine.hpp>

#if OVB_HAS_MK20
#include <overboard/hal/mk20/input.hpp>
#include <overboard/hal/mk20/keymap.hpp>
#include <overboard/mpad/device.hpp>
#endif

namespace ovb::hal::sdl {

/**
 * @brief SDL simulator application implementation
 *
 * Manages the SDL lifecycle for desktop simulation. Creates a single
 * merged window with LCD at top and keyboard at bottom, handles the
 * main event loop, and routes key events to the calculator engine.
 */
class SDL_App : public I_App {

    public:

        /// @brief Display width
        static constexpr int DISPLAY_WIDTH = ovb::hal::FULL_WIDTH;

        /// @brief Display height
        static constexpr int DISPLAY_HEIGHT = ovb::hal::FULL_HEIGHT;

        /**
         * @brief Factory method to create and initialize SDL application
         * @param layout Keyboard layout for the target device
         * @param layout_path Path to keyboard.json file
         * @return Unique pointer to initialized SDL_App, or nullptr on failure
         */
        static std::unique_ptr<SDL_App> create(const core::Grid_Layout& layout,
                                               const std::filesystem::path& layout_path);

        /**
         * Destructor
         */
        ~SDL_App() override;

        /**
         * @brief Initialize SDL application
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
         * @brief Handle a key press event from macropad (via keyboard.json)
         * @param key_index Index of the pressed key
         */
        void handle_key(int key_index);

        /**
         * @brief Handle input from standard keyboard
         * @param key Input_Key to pass to GUI for context-dependent handling
         */
        void handle_direct_action(core::Input_Key key);

        /**
         * @brief Single frame update: pump events, handle input, render LVGL
         * Used by the emscripten main loop on the WASM target.
         */
        void run_frame();

    private:

        /**
         * @brief Private constructor - use create() factory method
         * @param layout Keyboard layout reference
         */
        explicit SDL_App(const core::Grid_Layout& layout);

        /// @brief Callback function for key clicks
        static void on_key_clicked(int key_index, void* user_data);

        /// @brief Reference to the keyboard layout
        const core::Grid_Layout&          m_layout;

        /// @brief Path to the layout file
        std::filesystem::path             m_layout_path;

        /// @brief Flag indicating if the application has been initialized
        bool                              m_initialized = false;
        bool                              m_should_quit = false;

        /// @brief Keymap for the calculator
        core::Keymap m_keymap;

        /// @brief Layer manager for the calculator
        core::Layer_Manager m_layers;

        /// @brief Calculator engine for mathematical operations
        math::Calc_Engine m_engine;

        /// @brief SDL window driver
        std::unique_ptr<Display>       m_display;

        /// @brief Interactive key mapping info panel (in main window below LCD)
        /// Declared before m_view so it is destroyed after m_view (reverse order)
        std::unique_ptr<gui::Key_Mapping_Info> m_key_mapping_info;

        /// @brief Active keymap presenter — on-screen panel or MK20 device
        gui::I_Keymap_View*                   m_keymap_view = nullptr;

        /// @brief Declared attached device from keyboard.json (if any)
        std::optional<io::Keyboard_Config::Device_Config> m_device_config;

#if OVB_HAS_MK20
        // MK20 members declared before m_view; destruction order is
        // reverse declaration: keymap → input → device (reader thread
        // joined last, after callbacks can no longer fire).

        /// @brief MK20 macropad connection (input + presenter share it)
        std::shared_ptr<mpad::Device>         m_mk20_device;

        /// @brief MK20 serial input source
        std::unique_ptr<mk20::Mk20_Input>     m_mk20_input;

        /// @brief MK20 keymap presenter (owns the device theme state)
        std::unique_ptr<mk20::Mk20_Keymap>    m_mk20_keymap;
#endif

        /// @brief LVGL application view (LCD + panel system)
        /// Declared after m_display so it is destroyed first (reverse order),
        /// ensuring all LVGL objects are deleted while the display is still alive
        std::unique_ptr<gui::App_View> m_view;

        /// @brief System info provider
        std::unique_ptr<SDL_System_Info> m_system_info;

        /// @brief SDL input for the calculator
        std::unique_ptr<SDL_Input>   m_input;

        /// @brief SDL keymap for the calculator
        SDL_Keymap m_sdl_keymap;

        /// @brief Maps key index to Input_Key (for NONE-action passthrough)
        std::vector<core::Input_Key> m_key_input_map;

};

} // namespace ovb::hal::sdl
