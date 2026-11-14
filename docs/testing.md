# Testing

Tests are organized by the surface they exercise, mirroring the source layout. `CHECK` and `Throws` throw on
the first failure; `Run` reports the number of checks and returns a process status. Every test executable is
registered with CTest.

## Layers

| Layer | Target | Covers |
| --- | --- | --- |
| Validation | `interaction.widget` | Documented ranges and throw sites, without a composition context where possible |
| Widget contracts | `interaction.widget` | `UiTest` fixtures for press, hover, motion, reorder, and gesture composition |
| Advanced flows | `interaction.advanced` | Cross-widget sequences: keyboard activation, transform gestures, drag/drop, swipe, scroll, semantics |
| Gallery smoke | `interaction.preview` | The full preview composition, theme buttons, key lookup, hover, click, and scroll-to-section |
| Public API | `interaction.api` | Includes the umbrella header, constructs one symbol from each family, and links the result |

`tests/widget/` is split by family:

- `validation_test.cpp`: cross-family constructor validation.
- `press_test.cpp`: press, hover, paint modifiers, and trigger feedback.
- `gesture_test.cpp`: pinch, long press, double click, and reorder.

`tests/advanced/` is intentionally one cross-widget suite. It exercises interactions that only make sense
together, such as focus traversal, multi-pointer transforms, and drag/drop routing. Add new advanced cases to
that suite until a family grows enough to justify another translation unit.

## Running

```sh
cmake -S . -B build -DHUXERUI_HOME=/path/to/HuxerUI \
  -DHUXERUI_INTERACTION_BUILD_TESTS=ON \
  -DHUXERUI_BUILD_TESTING_LIBRARY=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The preview test needs packaged resources; it receives `INTERACTION_TEST_RESOURCES` from
`HuxerUI::huxerui` in `tests/CMakeLists.txt`. Widget and API tests work without the testing library, but the
composed-widget and preview tests are enabled only when `HuxerUI::testing` exists.

## Conventions

- Use stable `Key` identities for anything the test queries or taps.
- Prefer `UiTest` interactions (`Tap`, `Drag`, `SendPointer`, `SendScroll`, `PressKey`) over direct state
  mutation so the real routing path is covered.
- Use `PumpAndSettle` for animations and explicit `Pump(duration)` only when a gesture requires a hold.
- Test observable geometry, text, enabled state, or callbacks. Do not assert implementation details such as
  node counts or animation internals.
- A regression test must fail for a plausible consumer-visible bug. Do not add tests that merely restate
  source text or default values.
- Keep test helpers in `tests/support/test.h`; family files should only contain their fixtures and checks.
