# Project Tasks

## Phase 1: Refactoring

**Goal:** Pass over the codebase and find opportunities to reduce the amount of code — deduplicate, delete dead code, consolidate helpers.

- [ ] **1.1** Merge similar file/history/paths helpers — e.g. `io/file_utils` vs ad-hoc file reads elsewhere; keep one
- [ ] **1.2** Math canvas leftovers:
  - [ ] **1.2.1** Derive canvas size internally — query `lv_obj_get_width/height(canvas)` instead of passing `width`/`height` parameters
  - [ ] **1.2.2** Move result string rendering into layout tree — currently drawn with hardcoded offsets, should be a proper layout region
- [ ] **1.3** Docs review — verify each doc is accurate vs. current code; prune stale content:
  - [ ] **1.3.1** `docs/architecture.md` — module tree vs. actual `src/` layout (missing `mpad/`, `apps/`, screens, `io/file_utils`; `display/` module deleted — was orphaned)
  - [ ] **1.3.2** `docs/custom_fonts.md`
  - [ ] **1.3.3** `docs/diagrams.md` — and `docs/diagrams/` contents still referenced
  - [ ] **1.3.4** `docs/hardware.md` — PicoCalc vs. Pi Zero coverage; MK20 section needed?
  - [ ] **1.3.5** `docs/layers.md` — keep in sync with `data/configs/*/keyboard.json`
  - [ ] **1.3.6** `docs/pico_usb_gadget.md`
  - [ ] **1.3.7** `docs/README.md` + `Doxyfile`/`myst.yml` — doc build still works, config current
  - [ ] **1.3.8** `docs/images/`, `docs/overboard.tag`, `doxygen_footer.html` — still needed?
- [ ] **1.4** Source documentation cleanup — sweep `src/` for stale/inaccurate docs:
  - [ ] **1.4.1** `@file`/`@brief` headers match what each file actually does (recent churn: settings stores, keymap, `file_utils`, `key_icon`, `app_common`, `mk20/factory`)
  - [ ] **1.4.2** Doc comments describing removed/changed behavior (e.g. per-file `s_logger` references, mf34/th33 target mentions, `display/` module)
  - [ ] **1.4.3** Doxygen build — `docs/Doxyfile` still configured correctly; regenerate `overboard.tag` or drop it if unused
  - [ ] **1.4.4** Function banner blocks present on all `.cpp` method definitions per style guide

---

## Phase 2: MK20

**Goal:** Finish the Waveshare MK20 macropad integration — screens, input, and rendering all correct on device.

- [ ] **2.1** Implement Calculator screen per `docs/layers.md` — numpad key table, right-dial F-mode rotation (None → F1…F5 → None), PgUp/PgDn second page, `C`/`Ans` placeholders
- [ ] **2.2** `pop_overlay` restores the layer theme even when a screen is active — should restore the active screen's theme (or verify intended)
- [ ] **2.3** Push `system_data` strip updates on-change, not every second (~10% serial chatter on Status)
- [ ] **2.4** Re-evaluate the blank-theme interstitial — if baked unique-path icons fully repaint, dropping it saves ~0.6 s per switch

---

## Phase 3: Calculator — Basic Algebra

**Goal:** Get the calculator working end-to-end for basic arithmetic and algebra.

**New Operations:**

*Absolute Value & Sign:*
- [ ] **3.1** `sign(x)` — sign function (returns -1, 0, or 1)

*Rounding & Truncation:*
- [ ] **3.2** `floor(x)` — largest integer ≤ x
- [ ] **3.3** `ceil(x)` — smallest integer ≥ x
- [ ] **3.4** `round(x)` — round to nearest integer
- [ ] **3.5** `trunc(x)` — truncate to integer (towards zero)
- [ ] **3.6** `frac(x)` — fractional part (x - floor(x))

*Number Theory:*
- [ ] **3.7** `gcd(a, b)` — greatest common divisor (Euclidean algorithm)
- [ ] **3.8** `lcm(a, b)` — least common multiple (a*b / gcd(a,b))
- [ ] **3.9** `is_prime(n)` — primality test (returns 1 if prime, 0 otherwise)
- [ ] **3.10** `prime_factors(n)` — returns list/count of prime factors (display as string)
- [ ] **3.11** `is_even(n)` — returns 1 if even, 0 if odd
- [ ] **3.12** `is_odd(n)` — returns 1 if odd, 0 if even

*Combinatorics:*
- [ ] **3.13** `nCr(n, r)` or `binom(n, r)` — binomial coefficient (n choose r)
- [ ] **3.14** `nPr(n, r)` or `perm(n, r)` — permutations (n!/(n-r)!)

*Min/Max:*
- [ ] **3.15** `min(a, b)` — minimum of two values
- [ ] **3.16** `max(a, b)` — maximum of two values
- [ ] **3.17** `clamp(x, min, max)` — constrain x to [min, max]

*Random Numbers:*
- [ ] **3.18** `rand()` — random float in [0, 1)
- [ ] **3.19** `randint(a, b)` — random integer in [a, b]
- [ ] **3.20** `seed(n)` — set random seed for reproducibility

*Modular Arithmetic:*
- [ ] **3.21** `div(a, b)` — integer division (floor(a/b))
- [ ] **3.22** `rem(a, b)` — remainder (a - b*div(a,b))
- [ ] **3.23** `mod_pow(base, exp, mod)` — modular exponentiation (base^exp mod m)
- [ ] **3.24** `mod_inv(a, m)` — modular multiplicative inverse

*Logarithms & Exponentials:*
- [ ] **3.25** `log2(x)` — base-2 logarithm
- [ ] **3.26** `logb(x, b)` — logarithm base b

*Internal Refactoring:*
- [ ] **3.27** **Function dispatch table** — replace the fixed `if`-chain in `Function_Node::eval()` (and the parallel chains in `to_latex()` and the parser) with a registered function table (see Phase 9 `Function_Registry`). Adding a new function currently requires edits in three separate places.

*Summation & Products:*
- [ ] **3.28** `sum(list)` — sum of array/list elements (future: requires array support)
- [ ] **3.29** `prod(list)` — product of array/list elements
- [ ] **3.30** `mean(list)` — arithmetic mean
- [ ] **3.31** `geomean(list)` — geometric mean

---

## Phase 4: Status Panel Update

**Goal:** Refresh the status panel with a clock widget and About popup.

- [ ] **4.1** Update F2 footer label to "Board"
- [ ] **4.2** Create Board Stats popup with system info (`I_System_Info` + SDL impl done):
  - [ ] **4.2.1** Verify on macOS (current)
  - [ ] **4.2.2** Verify on Linux
- [ ] **4.3** CPU temperature:
  - [ ] **4.3.1** PicoCalc: RP2350 internal temperature sensor (ADC)
  - [ ] **4.3.2** Pi Zero: `/sys/class/thermal/thermal_zone0/temp` (implemented, needs verify)
- [ ] **4.4** Battery voltage:
  - [ ] **4.4.1** PicoCalc: MAX17048 via I2C
  - [ ] **4.4.2** Pi Zero: LiPo HAT via power_supply sysfs
- [ ] **4.5** Storage stats:
  - [ ] **4.5.1** PicoCalc: SD card FAT32
  - [ ] **4.5.2** Pi Zero: SD card (implemented, needs verify)
- [ ] **4.6** USB/BT status:
  - [ ] **4.6.1** PicoCalc: TinyUSB + optional BT module
  - [ ] **4.6.2** Pi Zero: USB gadget mode, hciconfig for BT


---

## Phase 5: Refactoring — Second Pass

**Goal:** A second cleanup pass after the MK20/Calculator/Status work lands — same sweep pattern as Phase 1, applied per module: deduplicate repeated code, remove dead code and stale `#ifdef`s, consolidate logging boilerplate, merge duplicate helpers, and verify doc comments match behavior.

- [ ] **5.1** `core/` — layout/layer/screen/input-key types; e.g. repeated parse helpers, unused `Action_Code`/`Input_Key` entries, dead layout factories
- [ ] **5.2** `io/` — `keyboard_config`, `file_utils`; check for ad-hoc file reads elsewhere that should route through it (finish 1.1 scope)
- [ ] **5.3** `display/` — canvas/layout/cursor abstraction; verify the layer still earns its indirection
- [ ] **5.4** `font/` — font metrics + generated font pipeline; dead glyphs/sizes, duplicate metric builders
- [ ] **5.5** `gui/` — `App_View`, `Panel_Manager`, `Key_Mapping_Info`, overlays, `math_canvas`; panel-change/callback wiring that may now be redundant with `app_common`
- [ ] **5.6** `apps/` — calculator, status (widgets + popups), settings; duplicated widget/popup scaffolding
- [ ] **5.7** `math/` — engine, parser, AST, layout tree; parallel if-chains (also see 3.27 function-dispatch table / Phase 9 `Function_Registry`)
- [ ] **5.8** `hal/` — sdl, pi_zero, picocalc, mk20 + shared helpers (`app_common`, `host_info`, `file_settings_store`, `mk20/factory`); any leftover per-target duplication
- [ ] **5.9** `mpad/` — `device`, `serial_port`, `protocol`, `theme`, `key_icon`; keep platform-neutral
- [ ] **5.10** `log/` — post-`file_logger` removal; verify `I_Logger` interface still earns its abstraction vs. direct `Stdout_Logger`
- [ ] **5.11** `tools/` — `ovt_*` utilities, `keylogger`, `test_math_render`; shared helpers across tools (arg parsing, PNG writing)
- [ ] **5.12** Misc — `protocol/` (usb_gadget only), `src/usb_gadget/`, `src/main.cpp`, CMake source lists vs. actual files, `.devin/rules` vs. current conventions

---

## Phase 6: Pico 2 W — USB + Bluetooth HID Gadget

**Goal:** Pico 2 W acts as USB HID keyboard when cabled; falls back to Classic Bluetooth HID when wireless.

- [ ] **6.1** **Compile + flash test** on physical Pico 2 W hardware (BT_HID, dual-transport routing, SDP, fixed PIN `"0000"` all implemented — see `docs/pico_usb_gadget.md`)
- [ ] **6.2** Pair with macOS/Linux host, verify keyboard + consumer control reports received over BT
- [ ] **6.3** Verify USB HID still works when cable is plugged in (transport switch)
- [ ] **6.4** ICD extension: add `BT_PIN_CHANGE` message (type `0x06`) to `uart_protocol.hpp` so Pi Zero can set PIN at runtime (see TODO in `bt_hid.hpp` and `docs/pico_usb_gadget.md`)

---

## Phase 7: Function-Key Popup System

**Goal:** F1–F10 on PicoCalc shows a popup anchored to the footer slot, with paging for F6-F10.

**Basic F-Key Support (F1-F5):**
- [ ] **7.1** Define `I_Popup` interface — `show()`, `hide()`, `handle_input()`, `render()`
- [ ] **7.2** Implement `Function_Key_Popup` — generic popup with scrollable item list
- [ ] **7.3** Map F1–F5 `Action_Code` values to footer slots
- [ ] **7.4** On F-key press: show matching popup via `Panel_Manager`
- [ ] **7.5** Popup dismissal: any non-F-key press or second press of same F-key closes it
- [ ] **7.6** Add unit tests for popup lifecycle

**Extended F-Key Support (F6-F10):**
- [ ] **7.7** Footer paging system:
  - [ ] **7.7.1** Add footer page state (page 0: F1-F5, page 1: F6-F10)
  - [ ] **7.7.2** PAGE_DOWN key advances to next footer page (F1-F5 → F6-F10)
  - [ ] **7.7.3** PAGE_UP key returns to previous footer page (F6-F10 → F1-F5)
  - [ ] **7.7.4** Visual indicator showing current footer page (e.g., "Page 1/2")
  - [ ] **7.7.5** Wrap-around behavior (optional): PAGE_DOWN on last page returns to first
- [ ] **7.8** Footer label updates:
  - [ ] **7.8.1** Dynamically update footer labels when page changes
  - [ ] **7.8.2** Show F6-F10 labels on page 1 (or blank if unassigned)
  - [ ] **7.8.3** Handle layer interactions (page state independent of layer state)
- [ ] **7.9** F6-F10 popup assignments:
  - [ ] **7.9.1** Map F6-F10 action codes to popup slots 5-9
  - [ ] **7.9.2** Ensure F6-F10 popups work identically to F1-F5
- [ ] **7.10** Edge cases and robustness:
  - [ ] **7.10.1** Closing popup doesn't reset footer page
  - [ ] **7.10.2** Layer switching preserves current footer page
  - [ ] **7.10.3** ESC or mode change can optionally reset to page 0
- [ ] **7.11** Add unit tests for:
  - [ ] **7.11.1** Footer page navigation (PAGE_UP/PAGE_DOWN)
  - [ ] **7.11.2** F6-F10 popup triggering on correct page
  - [ ] **7.11.3** Page state persistence across popups and layers

---

## Phase 8: F-Key Contexts

**Goal:** PgUp/PgDn cycles through named contexts, each defining what F1–F5 do and their labels in the footer bar. Each F-key opens a popup menu with context-specific items.

**Core Context Infrastructure:** (struct, context vector, `cycle_context()`, PgUp/PgDn wiring done)
- [ ] **8.1** Update footer bar to show context name and page indicator (e.g. "Core Math 1/4")

**Context Definitions (Default Layer):** (Core Math done)
- [ ] **8.2** Number Theory context: F1=Factor, F2=Divis, F3=Mod, F4=Base, F5=Comb
- [ ] **8.3** Statistics context: F1=Avg, F2=Spread, F3=Dist, F4=Fit, F5=Misc
- [ ] **8.4** Units & Conversions context: F1=Len, F2=Mass, F3=Temp, F4=Area, F5=Speed

**Popup Item Implementation:**
- [ ] **8.5** Core Math popups: Logarithms and Rounding items
- [ ] **8.6** Number Theory popups: factorize, is_prime, GCD, LCM, mod, base conversion, nCr, nPr
- [ ] **8.7** Statistics popups: mean, median, std dev, normal CDF, regression, sum/min/max
- [ ] **8.8** Units & Conversions popups: length, mass, temperature, area, speed conversions

**Layer 2 (F6–F10):**
- [ ] **8.9** Define second-bank contexts for F6–F10 using the same physical keys on Layer 2
- [ ] **8.10** Ensure context switching works on Layer 2 independently or as a separate bank

**UI / Polish:**
- [ ] **8.11** Footer labels update when context changes
- [ ] **8.12** Popup state resets safely when switching contexts
- [ ] **8.13** Add unit tests for context cycling and popup assignment

---

## Phase 9: Catalog System

**Goal:** Universal catalog of all built-in functions, constants, and variables, accessible as both a standalone app and a calculator popup.

**Function Registry:**
- [ ] **9.1** `Function_Registry` class — central repository for all math functions
  - [ ] **9.1.1** Register function metadata: name, symbol, description, operand count, category
  - [ ] **9.1.2** Categories: Arithmetic, Trigonometry, Hyperbolic, Logarithmic, Statistics, Linear Algebra, Constants, Variables
  - [ ] **9.1.3** Support registration at initialization (static registration pattern)
  - [ ] **9.1.4** Query interface: search by name, filter by category, get all functions
- [ ] **9.2** Auto-register existing functions:
  - [ ] **9.2.1** All function operators (`Sin`, `Cos`, `Sqrt`, `Log`, etc.)
  - [ ] **9.2.2** Constants (`Pi`, `E`, `Phi`, `Tau`)
  - [ ] **9.2.3** Future: User-defined functions and variables
- [ ] **9.3** Function metadata structure:
  - [ ] **9.3.1** Name (display string)
  - [ ] **9.3.2** Action code (for insertion)
  - [ ] **9.3.3** Description (help text)
  - [ ] **9.3.4** Category (for filtering)
  - [ ] **9.3.5** Operand count and signature (e.g., "sin(x)", "atan2(y, x)")

**Catalog App (Standalone):**
- [ ] **9.4** `Catalog_App` class — browsable list of all registered functions
  - [ ] **9.4.1** Scrollable list view with search/filter
  - [ ] **9.4.2** Category tabs or dropdown for filtering
  - [ ] **9.4.3** Display: function name, signature, brief description
  - [ ] **9.4.4** Select function to view detailed help (syntax, examples, domain/range)
  - [ ] **9.4.5** Action: Copy function signature to clipboard (or return to calc with function inserted)
- [ ] **9.5** Register catalog app in app menu:
  - [ ] **9.5.1** Icon: `LV_SYMBOL_LIST` or custom catalog icon
  - [ ] **9.5.2** Hotkey: 'f' (for functions) or 't' (for tools)
  - [ ] **9.5.3** Menu priority: place after Calculator, before Settings
- [ ] **9.6** Search functionality:
  - [ ] **9.6.1** Text input field for filtering functions by name
  - [ ] **9.6.2** Incremental search (update list as user types)
  - [ ] **9.6.3** Search across name, description, and category

**Catalog Popup (In Calculator):**
- [ ] **9.7** Add catalog popup to calculator F-key menu (e.g., F5 or F9)
  - [ ] **9.7.1** Reuse `Function_Registry` for data
  - [ ] **9.7.2** Compact view: show only function names, no descriptions
  - [ ] **9.7.3** Numbered shortcuts (1-9) for quick selection
  - [ ] **9.7.4** Selecting function inserts it into expression at cursor
- [ ] **9.8** Integration with calculator:
  - [ ] **9.8.1** Insert function with placeholder arguments (e.g., `sin(□)`)
  - [ ] **9.8.2** Position cursor on first placeholder for immediate entry
  - [ ] **9.8.3** Close popup after insertion
- [ ] **9.9** Category filtering in popup:
  - [ ] **9.9.1** Arrow keys or tabs to switch categories
  - [ ] **9.9.2** Show category name in popup title (e.g., "Catalog: Trig")

**Help System Integration:**
- [ ] **9.10** Function help viewer:
  - [ ] **9.10.1** Display syntax, parameters, description
  - [ ] **9.10.2** Show examples with expected results
  - [ ] **9.10.3** Include domain/range restrictions (e.g., "asin: -1 ≤ x ≤ 1")
  - [ ] **9.10.4** Link to related functions (e.g., sin → asin, sinh)
- [ ] **9.11** Access help from catalog:
  - [ ] **9.11.1** Long-press or secondary action on function shows help
  - [ ] **9.11.2** Help displayed in overlay or separate panel

**Unit Tests:**
- [ ] **9.12** Function registry:
  - [ ] **9.12.1** Registration and query
  - [ ] **9.12.2** Category filtering
  - [ ] **9.12.3** Search functionality
- [ ] **9.13** Catalog app:
  - [ ] **9.13.1** List rendering
  - [ ] **9.13.2** Search and filter
  - [ ] **9.13.3** Function selection
- [ ] **9.14** Catalog popup:
  - [ ] **9.14.1** Integration with calculator
  - [ ] **9.14.2** Function insertion with placeholders

---

## Phase 10: Variable Support

Goal: Add user-defined variables with storage, recall, and management through the catalog system.

**Variable Infrastructure:**
- [ ] **10.1** Variable storage in `Calc_State`:
  - [ ] **10.1.1** `std::map<std::string, double> variables` — symbol table
  - [ ] **10.1.2** Reserve system variable names (e.g., `ans`, `pi`, `e`)
  - [ ] **10.1.3** Variable name validation (alphanumeric, starts with letter, case-sensitive)
- [ ] **10.2** `Variable_Node` evaluation context:
  - [ ] **10.2.1** Extend `eval()` to accept evaluation context with variable bindings
  - [ ] **10.2.2** Return variable value from context, or NaN if undefined
  - [ ] **10.2.3** Update all node `eval()` signatures to pass context through recursion
- [ ] **10.3** Variable assignment syntax:
  - [ ] **10.3.1** `x = 5` creates/updates variable `x` with value 5
  - [ ] **10.3.2** `x = sin(pi/4)` evaluates RHS and stores result
  - [ ] **10.3.3** Assignment returns the assigned value (for display)

**Variable UI:**
- [ ] **10.4** Variable menu/popup:
  - [ ] **10.4.1** List all defined variables with current values
  - [ ] **10.4.2** Show as "Vars" in F-key menu (e.g., F6 or dedicated key)
  - [ ] **10.4.3** Numbered shortcuts for quick recall
  - [ ] **10.4.4** Selecting variable inserts its name into expression
- [ ] **10.5** Variable management actions:
  - [ ] **10.5.1** Delete variable: select and press DELETE
  - [ ] **10.5.2** Clear all variables: confirmation dialog
  - [ ] **10.5.3** Rename variable (optional): edit name in-place
- [ ] **10.6** Variable assignment workflow:
  - [ ] **10.6.1** Type variable name (letter keys when not in number entry mode)
  - [ ] **10.6.2** Press `=` to enter assignment mode
  - [ ] **10.6.3** Enter expression for RHS
  - [ ] **10.6.4** Press EVAL to complete assignment and display result

**Catalog Integration:**
- [ ] **10.7** Register variables in `Function_Registry`:
  - [ ] **10.7.1** Variables appear in "Variables" category
  - [ ] **10.7.2** Show variable name and current value
  - [ ] **10.7.3** Update catalog dynamically when variables are added/deleted
- [ ] **10.8** Variable recall from catalog:
  - [ ] **10.8.1** Select variable from catalog inserts name into expression
  - [ ] **10.8.2** Show current value in preview/description area
- [ ] **10.9** System vs. user variables:
  - [ ] **10.9.1** System variables (`ans`, `pi`, `e`) marked read-only
  - [ ] **10.9.2** User variables shown with edit/delete options

**Special Variables:**
- [ ] **10.10** `ans` — stores result of last evaluation
  - [ ] **10.10.1** Auto-updated after each EVAL
  - [ ] **10.10.2** Read-only (cannot be assigned by user)
  - [ ] **10.10.3** Available immediately for use in next expression
- [ ] **10.11** Reserved constants:
  - [ ] **10.11.1** `pi`, `e`, `phi`, `tau` treated as read-only variables
  - [ ] **10.11.2** Attempting assignment shows error "Cannot assign to constant"

**Persistence (Optional):**
- [ ] **10.12** Save variables to settings/config file
- [ ] **10.13** Restore variables on app startup
- [ ] **10.14** Export variables to file (CSV or JSON)
- [ ] **10.15** Import variables from file

**Unit Tests:**
- [ ] **10.16** Variable storage and retrieval
- [ ] **10.17** Assignment parsing and execution
- [ ] **10.18** Variable evaluation in expressions
- [ ] **10.19** Special variable behavior (`ans`)
- [ ] **10.20** Catalog integration (registration, filtering)
- [ ] **10.21** Edge cases:
  - [ ] **10.21.1** Undefined variable returns NaN
  - [ ] **10.21.2** Reassignment updates value
  - [ ] **10.21.3** Reserved name rejection

---

## Phase 11: History Navigation and Management

Goal: TI-Nspire style history navigation allowing selective recall of expressions or results, plus history management.

**History Navigation (TI-Nspire Style):**
- [ ] **11.1** Add history navigation state to `Calc_Engine`:
  - [ ] **11.1.1** Current history index (which entry is selected)
  - [ ] **11.1.2** Navigation mode flag (browsing vs. editing)
  - [ ] **11.1.3** Selected content type (expression vs. result)
- [ ] **11.2** **UP arrow**: Navigate to previous history entry (move back in time)
- [ ] **11.3** **DOWN arrow**: Navigate to next history entry (move forward in time), or exit history mode if at newest
- [ ] **11.4** **LEFT arrow**: Insert the **expression** from current history entry into editor
- [ ] **11.5** **RIGHT arrow**: Insert the **result** from current history entry into editor
- [ ] **11.6** Visual feedback:
  - [ ] **11.6.1** Highlight selected history entry in the history table
  - [ ] **11.6.2** Show "History Mode" indicator in status area
  - [ ] **11.6.3** Preview expression/result that will be inserted
- [ ] **11.7** Clear history navigation mode on:
  - [ ] **11.7.1** Any edit action (typing, delete, etc.)
  - [ ] **11.7.2** ESC key
  - [ ] **11.7.3** Successful insertion (LEFT or RIGHT arrow)
- [ ] **11.8** Add unit tests for:
  - [ ] **11.8.1** History navigation state transitions
  - [ ] **11.8.2** Expression vs. result insertion
  - [ ] **11.8.3** Mode clearing on edits

**History Management:**
- [ ] **11.9** Delete individual history entries:
  - [ ] **11.9.1** While in history navigation mode, DELETE key removes selected entry
  - [ ] **11.9.2** Confirm deletion with visual feedback (fade out animation)
  - [ ] **11.9.3** Update history indices after deletion
- [ ] **11.10** Clear all history:
  - [ ] **11.10.1** Add "Clear History" action code
  - [ ] **11.10.2** Add confirmation dialog for clearing all entries
  - [ ] **11.10.3** Preserve current expression when clearing history
- [ ] **11.11** History size management:
  - [ ] **11.11.1** Enforce MAX_HISTORY limit (currently 20)
  - [ ] **11.11.2** Auto-remove oldest entries when limit reached
  - [ ] **11.11.3** Optional: Make MAX_HISTORY user-configurable via settings
- [ ] **11.12** Add unit tests for:
  - [ ] **11.12.1** Individual entry deletion
  - [ ] **11.12.2** History clearing
  - [ ] **11.12.3** Size limit enforcement

---

## Phase 12: Array and Matrix Support

Goal: Add first-class support for 1D arrays (vectors) and ND arrays (matrices) with basic linear algebra operations.

**Data Structures:**
- [ ] **12.1** `Array_Node` AST node — holds dimensions, data storage, and shape metadata
- [ ] **12.2** `Matrix` value type — stores elements in row-major order with dimension tracking
- [ ] **12.3** Array literal syntax — `[1, 2, 3]` for vectors, `[[1,2], [3,4]]` for matrices
- [ ] **12.4** Placeholder templates — `[□, □, □]` for quick vector entry, matrix templates for common sizes

**Construction & Access:**
- [ ] **12.5** Array constructor functions (`zeros`/`ones`/`eye` done):
  - [ ] **12.5.1** `diag([v1, v2, ...])` — diagonal matrix from vector
  - [ ] **12.5.2** `linspace(start, stop, n)` — linearly spaced vector
  - [ ] **12.5.3** `arange(start, stop, step)` — range vector with step size
- [ ] **12.6** Element access — `A[i, j]` notation for matrices, `v[i]` for vectors
- [ ] **12.7** Submatrix extraction — `A[1:3, 2:4]` slice syntax
- [ ] **12.8** Shape queries — `size(A)`, `rows(A)`, `cols(A)`, `length(v)`

**Basic Operations:**
- [ ] **12.9** Element-wise arithmetic — `A + B`, `A - B`, `A .* B`, `A ./ B` (broadcasting rules)
- [ ] **12.10** Scalar operations — `k * A`, `A / k`, `A + k`
- [ ] **12.11** Matrix multiplication — `A * B` (standard matrix product)
- [ ] **12.12** Transpose — `A'` or `transpose(A)`
- [ ] **12.13** Concatenation — `hcat(A, B)`, `vcat(A, B)` for horizontal/vertical stacking
- [ ] **12.14** Reshape — `reshape(A, m, n)` to change dimensions while preserving data

**Reduction Operations:**
- [ ] **12.15** `sum(A)` — sum of all elements, or along dimension
- [ ] **12.16** `prod(A)` — product of all elements
- [ ] **12.17** `mean(A)` — arithmetic mean
- [ ] **12.18** `std(A)` — standard deviation
- [ ] **12.19** `min(A)` / `max(A)` — extrema
- [ ] **12.20** `trace(A)` — sum of diagonal elements
- [ ] **12.21** `det(A)` — determinant (2×2 and 3×3 direct formulas)
- [ ] **12.22** `norm(v)` — Euclidean norm of vector

**Display & Rendering:** (typeset layout + `Dimension_Picker_Popup` done)
- [ ] **12.23** Compact notation for large matrices — show corners with ellipsis
- [ ] **12.24** Vector display — horizontal `[1, 2, 3]` or vertical column format

**Unit Tests:** (`insert_matrix`/`insert_vector` done in `test_matrix_ops.cpp`)
- [ ] **12.25** Element access and slicing
- [ ] **12.26** Arithmetic operations with dimension checks
- [ ] **12.27** Matrix multiplication correctness
- [ ] **12.28** Reduction operations accuracy

---

## Phase 13: Advanced Linear Algebra

Goal: Implement matrix factorizations, system solvers, and advanced decomposition methods for scientific computing.

**Matrix Factorizations:**
- [ ] **13.1** **LU Decomposition** — `[L, U, P] = lu(A)` with partial pivoting
  - [ ] **13.1.1** Solve `Ax = b` via forward/backward substitution
  - [ ] **13.1.2** Compute determinant via diagonal product of U
- [ ] **13.2** **QR Decomposition** — `[Q, R] = qr(A)` using Householder reflections or Gram-Schmidt
  - [ ] **13.2.1** Least squares solver `x = qr_solve(A, b)`
  - [ ] **13.2.2** Orthonormalization of column space
- [ ] **13.3** **Cholesky Decomposition** — `L = chol(A)` for symmetric positive-definite matrices
  - [ ] **13.3.1** Efficient solver for symmetric systems
  - [ ] **13.3.2** Numerical stability checks

**Eigenvalue Problems:**
- [ ] **13.4** **Power iteration** — dominant eigenvalue and eigenvector
- [ ] **13.5** **QR algorithm** — compute all eigenvalues iteratively
- [ ] **13.6** **Characteristic polynomial** — symbolic or numeric roots for small matrices
- [ ] **13.7** `eig(A)` function — return eigenvalues and eigenvectors
- [ ] **13.8** `eigvals(A)` — eigenvalues only

**Singular Value Decomposition (SVD):**
- [ ] **13.9** **SVD factorization** — `[U, S, V] = svd(A)` where `A = U * S * V'`
- [ ] **13.10** Golub-Reinsch algorithm for numerical SVD
- [ ] **13.11** Truncated SVD for low-rank approximation
- [ ] **13.12** Condition number — `cond(A) = σ_max / σ_min`
- [ ] **13.13** Matrix rank — `rank(A)` via singular value threshold

**Linear System Solvers:**
- [ ] **13.14** **Direct solver** — `x = A \ b` using LU decomposition
- [ ] **13.15** **Least squares** — `x = lsq(A, b)` for overdetermined systems (QR or SVD)
- [ ] **13.16** **Pseudo-inverse** — `A_pinv = pinv(A)` via SVD
- [ ] **13.17** **Iterative solvers** (optional):
  - [ ] **13.17.1** Conjugate gradient for symmetric positive-definite systems
  - [ ] **13.17.2** GMRES for general systems
  - [ ] **13.17.3** Preconditioners for improved convergence

**Matrix Analysis:**
- [ ] **13.18** Spectral radius — `ρ(A) = max(|λ_i|)`
- [ ] **13.19** Matrix norms — Frobenius, operator norms (1, 2, ∞)
- [ ] **13.20** Null space basis — `null(A)` via SVD
- [ ] **13.21** Range (column space) basis — `range(A)` via QR or SVD

**Advanced Operations:**
- [ ] **13.22** Matrix exponential — `expm(A)` via Padé approximation or eigendecomposition
- [ ] **13.23** Matrix power — `A^k` for integer k
- [ ] **13.24** Kronecker product — `kron(A, B)`
- [ ] **13.25** Vectorization — `vec(A)` to stack columns

**Numerical Robustness:**
- [ ] **13.26** Pivoting strategies for LU (partial, full)
- [ ] **13.27** Singular matrix detection and error handling
- [ ] **13.28** Ill-conditioned matrix warnings
- [ ] **13.29** Iterative refinement for improved accuracy

**Unit Tests:**
- [ ] **13.30** Factorization correctness (reconstruction check: `A ≈ L*U`)
- [ ] **13.31** System solver validation against known solutions
- [ ] **13.32** SVD properties (orthogonality of U and V)
- [ ] **13.33** Pseudo-inverse uniqueness and Moore-Penrose properties
- [ ] **13.34** Numerical stability on ill-conditioned matrices

---

## Phase 14: Core Features

Goal: Session export and a dedicated symbol layer.

- [ ] **14.1** **Export C** — button to export the session history as a Python script (each entry as a commented expression + assignment) or a LaTeX document (formatted as an `align` environment)
- [ ] **14.2** **Symbol layer** — dedicated layer for inserting named symbols: Greek alphabet (α β γ δ … ω), uppercase variants, and Latin letter variables (a–z)

---

## Phase 15: Math Rendering

Goal: Typeset display of evaluated expressions using the AST, shown after `=` is pressed (TI-Nspire style).

- [ ] **15.1** `Math_Box` layout model — width, height, baseline offset per node; layout pass separate from paint pass
- [ ] **15.2** Stage 1: numbers, binary `+`/`−`, fractions (`/` → stacked numerator/denominator with rule)
- [ ] **15.3** Stage 2: `^` superscript (recursive scale parameter), `sqrt` with dynamic vinculum
- [ ] **15.4** Stage 3: functions (`sin`, `cos`, etc.) with parenthesised arguments
- [ ] **15.5** Hook `Math_Renderer` into `Display_Controller::render_lcd()` post-equals, using `last_ast`

---

## Phase 16: Symbolic Math (CAS)

Goal: NSpire-style exact/approximate mode — all evaluation is symbolic (`simplify()`); pressing `≈` wraps the expression in `approx(…)` which force-evaluates to a decimal. `Node::simplify()` / `Node::clone()` scaffolding in place.

- [ ] **16.1** `Rational` type for exact integer arithmetic (`1/3 + 1/6 = 1/2`)
- [ ] **16.2** Symbolic radicals — keep `sqrt(2)` unevaluated unless inside `approx()`
- [ ] **16.3** Variable binding — symbol table in `Calc_State`, variable tokens via the symbol layer
- [ ] **16.4** Differentiation — recursive AST transform `Node::differentiate(var)`

---

## Phase 17: System Improvements

Goal: Cross-cutting quality — debugging, portability, error handling, UI polish.

- [ ] **17.1** **Debugger flag** — ASan via `-DUSE_ASAN=ON` for memory corruption debugging; check OS/compiler, adjust flags accordingly
- [ ] **17.2** **WASM/Emscripten build target** — EMSDK setup, `TARGET_DEVICE=WASM` CMake path, browser launch via `emrun`
- [ ] **17.3** **Remove exceptions for embedded builds** — ARM toolchain disables exceptions by default; replace all `throw` statements with error codes, assertions, or std::optional returns throughout codebase
  - [ ] **17.3.1** **Establish error handling policy** — define how to handle unrecoverable errors (e.g., placeholder_node::eval(), division by zero) without exceptions: assertions for programming errors, error codes for I/O failures, special return values (NaN) for math errors
- [ ] **17.4** **AST display refactor** — render expressions using layout engine (fractions, superscripts, etc.)
- [ ] **17.5** **ATAN2** action code for two-argument arctangent
- [ ] **17.6** **Animation** — smooth cursor transitions in LVGL
- [ ] **17.7** **Themes** — customisable colours and fonts
