# Testing

Tests mirror the library's family architecture and layer boundaries. `CHECK` and `Throws` fail immediately on the first
broken invariant; `Run` reports check counts and returns a process exit code. Every test executable is registered with
CTest and compiled with `-Wall -Wextra -Wpedantic` (or `/W4` on MSVC).

## Layers

| Layer | CTest Name | Target | Covers |
| --- | --- | --- | --- |
| Public API & Static Contracts | `interaction.api` | `interaction_api_tests` | Umbrella include, `std::equality_comparable` static assertions on all modifiers/controllers/triggers, runtime-free default handle equality, and link smoke across all families |
| Validation & Family Contracts | `interaction.widget` | `interaction_widget_tests` | Centralized constructor/range throw checks plus per-family `UiTest` behavioral suites |
| Cross-Widget Integration | `interaction.advanced` | `interaction_advanced_tests` | Multi-stage `operator|` pipelines, mutually exclusive `SwipeController` list rows, `Reorderable` (`handle_only`) + `ReorderHandle` + child button isolation, external `PanZoomController` toolbars, `RovingFocusGroup` + `Popover` + `BackHandler` priority, `MarqueeSelectArea` + `Copyable` + `ShortcutMap`, and `ScrollHideController` + `ScrollSpy` + `Expandable` geometry sync |
| Gallery Smoke | `interaction.preview` | `interaction_preview_tests` | Full preview app linked via shared `interaction_preview_views`, testing hover bloom, boop counter, theme toggles, and scrolling |

`tests/widget/` is organized by responsibility:

- `validation_test.cpp`: cross-family invalid-argument contracts without an active composition context.
- `press_test.cpp`: `InteractiveSurface`, `Pressable` (primary-button filter, keyboard Space & focus-loss recovery),
  `Copyable`, `LongPressable` + `HoldProgressRing`, `DoubleClickable`, `MultiClickable`, `ContextMenuArea`,
  `SecondaryClickable`, `Ripple`.
- `hover_overlay_test.cpp`: `Hoverable`, `HoverLift`, `HoverShadow`, `Magnetic`, `Spotlight`, `HoverHighlight`,
  `HoverCard` + `HoverCardController`, `Popover` + `PopoverController`.
- `gesture_test.cpp`: `PinchZoom`, `PanZoom`, `InertialDrag`, `MarqueeSelectArea`/`MarqueeSelectItem`, `SwipeActions` +
  `SwipeActionContext`, `Dismissible` (custom background builder), `PressDragHandle`, `ResizeHandle`.
- `reorder_dragdrop_test.cpp`: vertical/horizontal `Reorderable`, `ReorderableGrid`, `Draggable<T>`, headless &
  predicate-filtered `DropZone<T>`, `FileDropZone`.
- `scroll_a11y_visual_test.cpp`: `ScrollProgress`, `ScrollEdgeFade`, `NestedScrollConnection` + `ScrollHideController`,
  `ScrollSpyArea`/`ScrollSpySection`/`ScrollSpyController`, `SemanticActions`, `KeyboardShortcut`, `ShortcutMap`,
  `BackHandler`, `PredictiveBack` (Escape commit + predictive `Begin`/`Update`/`Cancel`/`Commit`), `RovingFocusGroup`/
  `RovingFocusItem`, `Expandable`, `Appear`, `SharedMorph`, `Pulsing`, `Shimmer`, `RollingNumber`, `UsePop`/`Popped`,
  `UseShake`/`Shaken`, `UseRevealOnTap`.

## Running

```sh
cmake -S . -B build -DHUXERUI_HOME=/path/to/HuxerUI \
  -DHUXERUI_INTERACTION_BUILD_TESTS=ON \
  -DHUXERUI_BUILD_TESTING_LIBRARY=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```
