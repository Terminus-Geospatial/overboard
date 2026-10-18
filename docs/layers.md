# MK20 Screens

<style>
kbd {
  border: 1px solid #ccc;
  border-radius: 3px;
  padding: 2px 6px;
  background-color: #f8f8f8;
  font-family: monospace;
  display: inline-block;
  min-width: 20px;
  text-align: center;
}
kbd.unused {
  opacity: 0.35;
}
.mk20-device {
  display: inline-flex;
  flex-direction: column;
  gap: 8px;
  margin: 10px 0;
  border: 3px solid #888 !important;
  border-radius: 6px;
  padding: 10px;
  background-color: transparent;
}
.mk20-top {
  display: grid;
  grid-template-columns: auto 1fr auto;
  gap: 10px;
  align-items: stretch;
}
.mk20-dial {
  border: 3px solid #888 !important;
  border-radius: 4px;
  padding: 4px 6px;
  background-color: transparent;
  display: flex;
  flex-direction: column;
  gap: 3px;
  align-items: center;
  justify-content: center;
}
.mk20-dial .dial-name {
  font-family: monospace;
  font-size: 0.75em;
  opacity: 0.6;
}
.mk20-screen {
  border: 3px solid #888 !important;
  border-radius: 4px;
  padding: 4px;
  background-color: transparent;
  display: flex;
  align-items: center;
  justify-content: center;
  font-family: monospace;
  font-size: 0.8em;
  opacity: 0.7;
  min-height: 56px;
}
.mk20-grid {
  display: grid;
  grid-template-columns: repeat(5, auto);
  grid-template-rows: repeat(4, auto);
  gap: 4px;
  border: 3px solid #888 !important;
  border-radius: 4px;
  padding: 4px;
  background-color: transparent;
}
</style>

## Contents

- [Screen — Menu](#screen--menu)
- [Screen — Status](#screen--status)
- [Screen — Calculator](#screen--calculator)
- [Remaining screens](#remaining-screens)

The MK20 presents one **screen** per active app panel. Each screen lays
out the device's three display areas:

- **428×142 status strip** — a glanceable readout driven by the app
- **2 dials** — rotary encoders flanking the strip
- **5×4 key grid** — 20 LCD keys showing the screen's actions

Conventions across screens:

- **Esc always sits at r0c0** (top-left) — one consistent "back" position.
- **Left dial navigates** — scrolls or cycles whatever the screen
  presents; its click is the screen's primary confirm.
- **Right dial adjusts** — host volume on rotate, mute on click
  (native device action). Identical on every screen.
- Unbound keys stay dim — no label, no action.

---

## Screen — Menu

Active when the **Menu** panel is focused — the root launcher listing
Status / Calculator / Settings. The pad becomes the launcher: one key
per app, dial to scroll.

<div class="mk20-device">
  <div class="mk20-top">
    <div class="mk20-dial">
      <span class="dial-name">L dial</span>
      <div><kbd>Up</kbd> <kbd>Sel</kbd> <kbd>Dn</kbd></div>
    </div>
    <div class="mk20-screen">overboard &nbsp;·&nbsp; v0.16.0 &nbsp;·&nbsp; 3 apps</div>
    <div class="mk20-dial">
      <span class="dial-name">R dial</span>
      <div><kbd>Vol-</kbd> <kbd>Mute</kbd> <kbd>Vol+</kbd></div>
    </div>
  </div>
  <div class="mk20-grid">
    <kbd>Esc</kbd> <kbd>Stat</kbd> <kbd>Calc</kbd> <kbd>Set</kbd> <kbd class="unused">&nbsp;</kbd>
    <kbd class="unused">&nbsp;</kbd> <kbd class="unused">&nbsp;</kbd> <kbd class="unused">&nbsp;</kbd> <kbd class="unused">&nbsp;</kbd> <kbd class="unused">&nbsp;</kbd>
    <kbd class="unused">&nbsp;</kbd> <kbd class="unused">&nbsp;</kbd> <kbd class="unused">&nbsp;</kbd> <kbd class="unused">&nbsp;</kbd> <kbd class="unused">&nbsp;</kbd>
    <kbd class="unused">&nbsp;</kbd> <kbd class="unused">&nbsp;</kbd> <kbd class="unused">&nbsp;</kbd> <kbd class="unused">&nbsp;</kbd> <kbd class="unused">&nbsp;</kbd>
  </div>
</div>

### Keys

| Key | Label | Behaviour |
|-----|-------|-----------|
| K1 | Esc | Back to the previous panel |
| K2 | Stat | Launch Status |
| K3 | Calc | Launch Calculator |
| K4 | Set | Launch Settings |
| K5–K20 | — | Reserved |

### Dials

| Dial | Gesture | Behaviour |
|------|---------|-----------|
| Left | rotate | Scroll the menu list |
| Left | click | Select highlighted item |
| Right | rotate | Host volume |
| Right | click | Host mute |

### Strip

`overboard · v0.16.0 · 3 apps` — app name, version, item count.

---

## Screen — Status

Active when the **Status** panel is focused. The strip is the
centerpiece: it mirrors the on-screen header widgets so the pad stays a
glanceable readout while the main display shows something else.

<div class="mk20-device">
  <div class="mk20-top">
    <div class="mk20-dial">
      <span class="dial-name">L dial</span>
      <div><kbd>Menu</kbd></div>
    </div>
    <div class="mk20-screen">clock &nbsp;·&nbsp; date &nbsp;·&nbsp; wifi</div>
    <div class="mk20-dial">
      <span class="dial-name">R dial</span>
      <div><kbd>Vol-</kbd> <kbd>Mute</kbd> <kbd>Vol+</kbd></div>
    </div>
  </div>
  <div class="mk20-grid">
    <kbd>Esc</kbd> <kbd>About</kbd> <kbd>Stats</kbd> <kbd>Net</kbd> <kbd>Calc</kbd>
    <kbd class="unused">&nbsp;</kbd> <kbd class="unused">&nbsp;</kbd> <kbd class="unused">&nbsp;</kbd> <kbd class="unused">&nbsp;</kbd> <kbd class="unused">&nbsp;</kbd>
    <kbd class="unused">&nbsp;</kbd> <kbd class="unused">&nbsp;</kbd> <kbd class="unused">&nbsp;</kbd> <kbd class="unused">&nbsp;</kbd> <kbd class="unused">&nbsp;</kbd>
    <kbd class="unused">&nbsp;</kbd> <kbd class="unused">&nbsp;</kbd> <kbd class="unused">&nbsp;</kbd> <kbd class="unused">&nbsp;</kbd> <kbd class="unused">&nbsp;</kbd>
  </div>
</div>

### Keys

| Key | Label | Behaviour |
|-----|-------|-----------|
| K1 | Esc | Close open popup; else back to Menu |
| K2 | About | Toggle About popup |
| K3 | Stats | Toggle Stats popup |
| K4 | Net | Toggle Connections popup |
| K5 | Calc | Jump straight to Calculator screen |
| K6–K20 | — | Reserved |

### Dials

| Dial | Gesture | Behaviour |
|------|---------|-----------|
| Left | rotate | — *(reserved — screen cycling is a good fit here later)* |
| Left | click | Back to Menu |
| Right | rotate | Host volume |
| Right | click | Host mute |

### Strip

`clock · date · wifi` — clock/date tick live; wifi shows the associated SSID.

---

## Screen — Calculator

Active when the **Calculator** panel is focused. Draft layout — the
base layer puts a ten-key on the left (columns c0–c2, all four rows).
Function modes `F1`–`F5` aren't keys — the **right dial** rotates
through `None → F1 → F2 → F3 → F4 → F5 → None`, so the freed slots go
to parens, paging, and editing keys.

> Note: `Esc` moves off r0c0 here to keep the numpad anchored at the
> top-left — breaks the cross-screen convention, revisit if the
> numpad lands somewhere else.

### Page 1 — entry

<div class="mk20-device">
  <div class="mk20-top">
    <div class="mk20-dial">
      <span class="dial-name">L dial</span>
      <div><kbd>&lt;</kbd> <kbd>=</kbd> <kbd>&gt;</kbd></div>
    </div>
    <div class="mk20-screen">expression &nbsp;·&nbsp; result preview</div>
    <div class="mk20-dial">
      <span class="dial-name">R dial</span>
      <div><kbd>Fn-</kbd> <kbd>·</kbd> <kbd>Fn+</kbd></div>
    </div>
  </div>
  <div class="mk20-grid">
    <kbd>7</kbd> <kbd>8</kbd> <kbd>9</kbd> <kbd>Esc</kbd> <kbd>Bsp</kbd>
    <kbd>4</kbd> <kbd>5</kbd> <kbd>6</kbd> <kbd>(</kbd> <kbd>)</kbd>
    <kbd>1</kbd> <kbd>2</kbd> <kbd>3</kbd> <kbd>C</kbd> <kbd>Ans</kbd>
    <kbd>0</kbd> <kbd>.</kbd> <kbd>=</kbd> <kbd>PgDn</kbd> <kbd>PgUp</kbd>
  </div>
</div>

### Keys

| Key | Label | Behaviour |
|-----|-------|-----------|
| K1–K3 | 7 8 9 | Digit entry (top numpad row) |
| K4 | Esc | Close menu overlay; else back to Menu |
| K5 | Bsp | Delete last character |
| K6–K8 | 4 5 6 | Digit entry |
| K9 | ( | Open paren |
| K10 | ) | Close paren |
| K11–K13 | 1 2 3 | Digit entry |
| K14 | C | Clear all (placeholder — shift as needed) |
| K15 | Ans | Last answer (placeholder — shift as needed) |
| K16 | 0 | Digit entry |
| K17 | . | Decimal point |
| K18 | = | Evaluate expression |
| K19 | PgDn | Switch to page 2 |
| K20 | PgUp | Switch to page 1 (wraps on page 1) |

### Page 2 — blank (TBD)

`PgDn` / `PgUp` swap views. Nothing assigned yet — `Esc` and the
paging keys stay live so you can get back.

<div class="mk20-device">
  <div class="mk20-top">
    <div class="mk20-dial">
      <span class="dial-name">L dial</span>
      <div><kbd>&lt;</kbd> <kbd>=</kbd> <kbd>&gt;</kbd></div>
    </div>
    <div class="mk20-screen">expression &nbsp;·&nbsp; result preview</div>
    <div class="mk20-dial">
      <span class="dial-name">R dial</span>
      <div><kbd>Fn-</kbd> <kbd>·</kbd> <kbd>Fn+</kbd></div>
    </div>
  </div>
  <div class="mk20-grid">
    <kbd>·</kbd> <kbd>·</kbd> <kbd>·</kbd> <kbd>Esc</kbd> <kbd>·</kbd>
    <kbd>·</kbd> <kbd>·</kbd> <kbd>·</kbd> <kbd>·</kbd> <kbd>·</kbd>
    <kbd>·</kbd> <kbd>·</kbd> <kbd>·</kbd> <kbd>·</kbd> <kbd>·</kbd>
    <kbd>·</kbd> <kbd>·</kbd> <kbd>·</kbd> <kbd>PgDn</kbd> <kbd>PgUp</kbd>
  </div>
</div>

### Dials

| Dial | Gesture | Behaviour |
|------|---------|-----------|
| Left | rotate | Move cursor / scroll expression |
| Left | click | Evaluate (same as `=`) |
| Right | rotate | Cycle F-mode: None → F1 → … → F5 → None |
| Right | click | TBD (was host mute) |

### Strip

`expression · result preview` — the current input line and the
evaluated result, mirroring the on-screen calculator display.

### F-modes

`F1`–`F5` are dial-selected function sets — while active, the key
grid (or a page's worth of it) rebinds to that mode's functions.
Contents TBD; candidates: arithmetic ops (`+ − × ÷`), trig,
constants (π e), memory/history, conversions.

### Open questions

- F-mode contents and whether the mode shows on the strip or as a
  badge.
- Right-dial click — confirm the F-mode selection? Back to host mute?
- `C` / `Ans` are placeholders; other candidates for the two open
  slots: `±`, `√`, `x²`, `%`.
- PgUp/PgDn order — currently `PgDn` sits left of `PgUp`.

---

## Remaining screens

- **Settings** — TBD
