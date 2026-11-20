# Repository structure

Lib-Interaction separates public interaction contracts from internal shared machinery, and separates input-driven
interaction primitives from pure visual/value-driven motion effects. Dependencies point strictly inward toward
HuxerUI core:

- **Layer 0 (`src/internal/`)**: Private validation, multi-controller frame aggregation, and shared slot-reorder
  geometry/session infrastructure. Never exposed in `include/`.
- **Layer 1 (Headless primitives, Controllers & Pipe)**: `InteractiveSurface`, `Hoverable`, builder-based `HoldRepeat` /
  `Copyable` / `Popover` / `PredictiveBack` / `StickyHeaderArea` / `ScrollParallax` / `ScrollReveal` / `ScrollToTop` /
  `SnapDrag` / `SwipeDeck` / `DropZone` / `FileDropZone` / `Dismissible`, `HoldRepeatController`, `SwipeController`,
  `SwipeDeckController`, `PanZoomController`, `HoverCardController`, `PopoverController`, `ScrollHideController`,
  `ScrollSpyController`, `ScrollToTopController`, `StickyHeaderController`, `SnapDragController`,
  `RovingFocusController`, `PredictiveBackController`, `MarqueeSelectController`, `ReorderHandle`,
  `ScrollToTopTrigger`, and `ViewAdaptor` (`operator|`).
- **Layer 2 (Styled presets, Retained modifiers & `visual/`)**: Ready-to-use composed widgets, retained
  `NodeExtension` paint/scroll/hover/focus/semantics modifiers, and non-interactive visual effects under `visual/`.

## Layout

```text
include/huxerui/
  interaction.h                       # canonical umbrella; exports all families and pipe combinators
  interaction/
    pipe.h                            # ViewAdaptor and left-to-right operator| pipeline support
    press/                            # InteractiveSurface, Pressable, HoldRepeat, HoldRepeatController, Copyable,
                                      # LongPressable, HoldProgressRing, MultiClickable, DoubleClickable,
                                      # ContextMenuArea, SecondaryClickable, Ripple
    hover/                            # Hoverable, HoverLift, HoverHighlight, HoverShadow, Spotlight, Magnetic
    gesture/                          # DragSpring, InertialDrag, SnapDrag, MarqueeSelectArea/Item, PanZoom, PinchZoom,
                                      # PanZoomController, SnapDragController, SwipeActions, SwipeController, SwipeDeck,
                                      # SwipeDeckController, Dismissible, PressDragHandle, ResizeHandle
    reorder/                          # Reorderable, ReorderableGrid, ReorderHandle
    dragdrop/                         # Draggable, DropZone (exact & predicate-filtered), FileDropZone
    overlay/                          # HoverCard, HoverCardController, Popover, PopoverController
    scroll/                           # ScrollProgress, ScrollEdgeFade, NestedScrollConnection, ScrollHideController,
                                      # ScrollParallaxArea, ScrollParallax, ScrollRevealArea, ScrollReveal,
                                      # ScrollSpyArea, ScrollSpySection, ScrollSpyController, ScrollToTop,
                                      # ScrollToTopTrigger, ScrollToTopController, StickyHeaderArea,
                                      # StickyHeaderSection, StickyHeaderList, StickyHeaderController
    accessibility/                    # SemanticActions, KeyboardShortcut, ShortcutMap, BackHandler,
                                      # PredictiveBack, RovingFocusGroup, RovingFocusItem, RovingFocusController
    visual/                           # Expandable, Appear, SharedMorph, Pulsing, Shimmer, Fireworks, RollingNumber,
                                      # UsePop/Popped, UseShake/Shaken, UseRevealOnTap (non-input visual/motion effects)
src/
  internal/                           # validate.h, motion_group.h, reorder_core.h/.cpp (private Layer 0)
  <family>/                           # mirrors include/huxerui/interaction/<family>/
examples/preview/
  PreviewViews.cmake                  # shared static target (interaction_preview_views) reused by app and tests
  src/
    app.cpp                           # gallery scaffold: theme, header, section order only
    demos/demos.h                     # cross-section DemoState counters + value-owned DemoContext
    demos/<family>.cpp                # autonomous section scopes with private assets and local state
tests/
  support/test.h                      # CHECK, Throws, Run, Center, Near
  widget/                             # family-aligned unit contracts + centralized validation_test.cpp
  advanced/                           # cross-widget pipelines, controller coordination, handle-only reorder
  preview/                            # gallery smoke test linked against interaction_preview_views
  api/api_surface_test.cpp            # umbrella compile/link smoke + equality_comparable static_asserts
docs/
  design.md                           # architecture, widget catalog, controller & motion conventions
  structure.md                        # this file
  testing.md                          # test layers and conventions
  open-questions.md                   # reviewed limitations and resolved design history
```

## Dependency and API rules

- `include/huxerui/interaction/*` may include fine-grained HuxerUI core headers and `pipe.h`, never `src/internal/`,
  `examples/`, or `tests/`. Triggers and controllers hide `State<T>` and `TaskScope` behind `std::shared_ptr` state
  structs so default-constructed handles are `noexcept`, runtime-free, and pointer-equality comparable.
- Every public `*Style` struct and every public retained `ViewModifier` (`Ripple`, `HoldProgressRing`,
  `HoverHighlight`, `Spotlight`, `Shimmer`, `Fireworks`, `ScrollProgress`, `ScrollEdgeFade`,
  `NestedScrollConnection`) must satisfy `std::equality_comparable` so `ModifierDescriptorFor` short-circuits
  unchanged updates.
- Wrapper functions follow one uniform parameter order:
  `(View content, [required domain arguments], primary_callback, Style style = {})`, with `std::invocable`-constrained
  overloads where a visual-only `(View content, Style style = {})` form is also meaningful.
- Stateless view decorators (`MultiClickable`, `DoubleClickable`, `ContextMenuArea`, `KeyboardShortcut`) attach
  modifiers directly without extra container nodes; apply them before stateful `Scope`-creating wrappers (`Pressable`,
  `HoverLift`) when piping both onto one node.
- `DemoState` in `examples/preview` holds only counters read across multiple sections (`StatusSection`). Section-local
  state must stay inside each section's own `Scope` so local interactions never recompose sibling sections.

## Build boundary

There is one library target, `huxerui_interaction` (`HuxerUI::Interaction`). Grouping directories organizes headers,
sources, and tests by responsibility while keeping linking and export simple.
