# MK20 Macropad Integration

Goal: replace the on-screen `Key_Mapping_Info` panel with a physical
Waveshare MK20 macropad (20 LCD keys in a 5×4 grid + 2 encoders +
428×142 secondary strip). The MK20 mirrors the active layer — its key
icons show layer labels, presses feed `handle_key()`, and the strip
can mirror calculator status.

Reference implementation: `terminus.mpad` Python driver in
`overboard_macropad_tools` (protocol docs in `docs/protocol.md` there).

## Verified timing (hardware, V2.30)

- Theme upload (~6.5 KB, 2 assets): **0.21 s**
- `reload_theme` (activate installed theme): **instant** — this is the
  fast layer-switch path. Upload one theme per layer at startup, then
  layer changes are free.
- `send_json` with a `main.currentPage` payload: sent, visual result
  unconfirmed — may allow page switching inside one theme without any
  re-upload at all.

## Design notes

- Input has two options:
  - **HID path (zero new code):** bind MK20 keys to `keyboard` actions
    emitting unused Input_Keys (F13–F24 etc.) → arrives via evdev →
    `Linux_Input` → `m_input_key_map` → `handle_key()`. Encoders the
    same way (`encoder_keyboard` → VOL_UP/DOWN or NEXT/PREV_LAYER).
  - **Serial path (proper):** `text` actions → serial events carry
    (row, col, down/up) explicitly; no HID leakage; needs protocol port.
- Output needs the serial protocol regardless (theme upload, label
  icons, `push_system_data` to the strip, `flash_key` support).
- Keep `ovb::mpad` platform-neutral (termios works on macOS + Linux) so
  the SDL simulator can drive the physical macropad during dev.
- Key icons: render labels offscreen via LVGL canvas → RGB565 → PNG
  via libpng (already a dependency). No new font/rendering deps.
- With `KBD_HEIGHT=0` + `SHOW_KEY_MAPPING=OFF`, `App_View` self-sizes
  to the full screen; `on_layer_change` / `set_panel_change_callback`
  hooks are where `Mk20_Keymap::update_layer()` attaches.

## Phase 1 — Protocol port (`src/overboard/mpad/`)

- [x] Serial client: termios (`serial_port.*`), frame codec (CRC32)
      in `protocol.*`, request/reply `Device::call`, reader thread
      → callbacks (`device.*`)
- [x] Theme encoder: tagged-map header, JSON layout blob, asset
      section (paths UTF-16-BE), trailer (`theme.*`)
- [x] Commands: FIND_DEVICE, FILE_START/END upload, SET_DEVICE_RELOAD,
      SEND_SYSTEM_DATA, SET_DEVICE_BL, SET_DEVICE_DELETE_THEME,
      SEND_JSON
- [x] Event dispatch: PROACTIVE_CMD key events (row/col, down/up)
- [x] `push_system_data`, `probe_command`, `on_frame` hook
- [ ] Unit tests against golden frames captured from Python side
- [ ] Key icon rendering: LVGL canvas → RGB → PNG (currently a shared
      embedded dark-slate PNG; per-key labels ride on `title`)

## Phase 2 — HAL integration

- [x] `data/configs/mk20/keyboard.json` — 5×4 grid (20 keys) + encoder
      pseudo-keys 20–25 bound to F13–F18; `"device": {"type":"mk20"}`
      block selects the presenter at runtime
- [x] `gui::I_Keymap_View` extracted; `Key_Mapping_Info` and
      `mk20::Mk20_Keymap` both implement it. `PiZero_App`/`SDL_App`
      hold `I_Keymap_View*` — MK20 wins if configured+connected,
      else on-screen panel (straight keyboards unaffected)
- [x] `Mk20_Keymap` — `update_layer()` lazily uploads one theme per
      layer then `reload_theme` switches instantly; overlay push/pop
      rebuilds a scratch theme; `flash_key()` is a no-op v1
- [x] `Mk20_Input` (`I_Input`) — serial events → `Key_Event{Action}`
      mapped (row,col)→layout index; encoder pseudo-rows 100/103 → 20/21
- [x] Encoder wiring: theme binds `encoder_keyboard` (L: F13/F17/F14,
      R: F15/F18/F16) → evdev/SDL → keys 20–25; `input_linux.cpp`
      extended to F24
- [x] `OVB_HAS_MK20` compile def + sources for SDL/PICOSDL/ZERO;
      `OVB_KEYBOARD_CONFIG` cache var (default `data/configs/mk20/`)
- [ ] Optional `TARGET_DEVICE=ZERO_MK20` + `KBD_HEIGHT=0` if the
      on-screen panel should be compiled out entirely (today it is
      runtime-skipped when the pad connects)

## Phase 3 — Secondary strip + polish

- [ ] Strip widgets: expression readout via `push_system_data` +
      type-113 text; gauges for memory/history count (optional)
- [ ] `flash_key` on press — brief icon swap or title push
- [ ] Panel overlays → overlay pages on MK20 (push_overlay/pop_overlay)
- [ ] Settings panel: backlight level control
- [ ] Handle disconnect/reconnect gracefully

## Open questions

- [ ] Does `send_json` switch pages in the active theme? (sent once,
      need to watch device — red→blue key test in
      `overboard_macropad_tools`)
- [ ] Theme upload time scales with assets — measure with real icon
      set (~20 PNGs)
- [ ] Key `titleParam` vs icon text — does the device render `title`
      over the icon, or must labels be baked into the PNG?
- [ ] Does the device survive serial disconnect/reconnect cleanly?
- [ ] Input choice for v1: HID-only (simplest) vs serial events
      (explicit row/col, no leakage)?
