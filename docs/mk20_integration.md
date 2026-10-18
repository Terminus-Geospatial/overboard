# MK20 Macropad Integration

Goal: drive a physical Waveshare MK20 macropad (20 LCD keys in a 5×4
grid + 2 encoders + 428×142 secondary strip) as the app's key
presenter. Per-panel "screens" (config section `screens` in
`data/configs/mk20/keyboard.json`) own key labels/icons, dial
bindings, and strip telemetry; layer themes remain the fallback for
panels without a screen.

Reference implementation: `terminus.mpad` Python driver in
`overboard_macropad_tools` (protocol docs in `docs/protocol.md` there).

## Verified timing (hardware, V2.30)

- Theme upload (~6.5 KB, 2 assets): **0.21 s**
- `reload_theme` (activate installed theme): **instant** — this is the
  fast screen-switch path. Upload once per screen/layer, then
  `reload_theme` swaps are free.
- `send_json` with a `main.currentPage` payload: sent, visual result
  unconfirmed — may allow page switching inside one theme without any
  re-upload at all.

## Remaining work

- [ ] **Theme corruption**: key text collapses onto the top two rows;
      strip shows unexpected `Text Text` + IP content and overflows
      into the key area. Diff generated `.Theme` JSON against the
      Python reference (geometry, item types, page structure).
- [ ] Unit tests against golden frames captured from Python side.
- [ ] `flash_key` on press — brief icon swap or title push.
- [ ] Settings panel: backlight level control (SET_DEVICE_BL).
- [ ] Handle serial disconnect/reconnect gracefully.
- [ ] Strip gauges for memory/history count (optional).
- [ ] Scan the new modules (`mpad/`, `hal/mk20/`, `io/` readers,
      `tools/ovt_icon_render`) for consolidation, simplification, or
      quality-of-life improvements — e.g. shared helpers vs. the
      Python-side equivalents, duplicated encoder/strip plumbing in
      `build_theme` vs `build_screen_theme`, and whether
      `push_system_data` channel names should be centralized.

## Open questions

- [ ] Does `send_json` switch pages in the active theme? (sent once,
      need to watch device — red→blue key test in
      `overboard_macropad_tools`)
- [ ] Theme upload time scales with assets — measure with real icon
      set (~20 PNGs).
- [ ] Does the device survive serial disconnect/reconnect cleanly?
