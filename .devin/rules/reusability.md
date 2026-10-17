---
trigger: always_on
---
# Reusability & Deduplication

When making changes to this codebase, flag opportunities that would
**increase reusability** or **decrease duplication** — don't silently
implement them, but don't skip noting them either.

## What to flag

- New code that resembles an existing module — suggest extracting a
  shared helper rather than adding a second copy.
- Platform-specific logic that could be portable — `hal/` code that
  only uses POSIX/standard APIs may belong in `core/` or a shared
  module (e.g. the `mpad` protocol client must stay platform-neutral
  so the SDL simulator can drive physical hardware).
- Interfaces that could abstract multiple backends — if a component
  has two+ implementations (e.g. `Key_Mapping_Info` vs an external
  key presenter), a shared interface is usually worth proposing.
- Repeated patterns across targets — similar init/teardown, config
  parsing, or event plumbing in `hal/sdl`, `hal/pi_zero`, `hal/mk20`.

## How to flag

- Mention it briefly in the response — what, where, and why it's
  worth extracting. Keep it to a sentence or two unless asked for more.
- Do NOT implement the refactor as part of an unrelated change unless
  explicitly approved; flag it as a follow-up instead.
