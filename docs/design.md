# Lib-Interaction design

This document describes the three-layer component architecture, the widget and controller catalog, and how each piece
maps onto HuxerUI's state, environment, and animation layers.

## Layered architecture

Lib-Interaction owns no rendering backend and no timing source; every animation flows through HuxerUI (`AnimateTo`,
`MotionController`, `TaskScope::Delay`). Internally, the library separates three concerns:

1. **Layer 0 — Private infrastructure (`src/internal/`)**: Shared numeric/duration validation (`validate.h`),
   multi-controller frame scheduling (`motion_group.h`), and slot-geometry / auto-scroll / keyboard-reorder machinery
   (`reorder_core.h`) shared between 1D `Reorderable` and 2D `ReorderableGrid`.
2. **Layer 1 — Headless state, Controllers, and Pipelines**:
   - **Headless builders** (`InteractiveSurface`, `Hoverable`, builder overloads of `DropZone<T>`, `FileDropZone`, and
     `Dismissible`) run the input/focus/semantics state machine and pass live state snapshots (`InteractionSnapshot`,
     `hovered`, `progress`) to caller-supplied view builders.
   - **Controllers** (`SwipeController`, `PanZoomController`, `HoverCardController`) use `std::shared_ptr` state
     handles remembered via `Use*Controller()`, enabling external state reads and programmatic actions (such as
     mutually exclusive swipe rows or zoom toolbars) while keeping default-constructed handles runtime-free.
   - **Environment-bridged grips & pipelines**: `ReorderHandle` connects to its enclosing `Reorderable` or
     `ReorderableGrid` slot via `Environment` (`ProvideEnvironment` / `UseEnvironment`), and `pipe.h` provides
     left-to-right `operator|` composition via `ViewAdaptor`.
3. **Layer 2 — Styled presets, Retained modifiers, and `visual/`**:
   - **Input-driven presets** (`Pressable`, `HoverLift`, `LongPressable`, `DragSpring`, `InertialDrag`, `PanZoom`,
     `PinchZoom`, `SwipeActions`, `Dismissible`, `Reorderable`, `ReorderableGrid`, `Draggable`, `DropZone`,
     `FileDropZone`, `HoverCard`) build default motion and chrome on top of Layer 1.
   - **Retained modifiers** (`Ripple`, `HoldProgressRing`, `Spotlight`, `HoverHighlight`, `ScrollProgress`,
     `ScrollEdgeFade`, `SemanticActions`) attach `NodeExtension` behavior through `View::With()` and implement
     `operator==` so unchanged recompositions skip extension updates.
   - **Visual/motion domain (`visual/`)** isolates non-interactive and value-driven presentation effects (`Pulsing`,
     `Shimmer`, `Fireworks`, `RollingNumber`, `UsePop`/`Popped`, `UseShake`/`Shaken`, `UseRevealOnTap`) from input
     recognizers.

## Catalog

| Component | Header family | Form | State / Control | Motion / Output |
| --- | --- | --- | --- | --- |
| `InteractiveSurface` | `press/surface.h` | Headless builder | `InteractionSnapshot{hovered, pressed, focused}` | Caller-defined |
| `Pressable` | `press/pressable.h` | Wrapper + `ViewAdaptor` | Built on `InteractiveSurface` (primary button + Space) | `Scale` around `origin` |
| `LongPressable` + `HoldProgressRing` | `press/longpress.h` | Wrapper + Modifier | Per-scope `holding` + `on_holding_changed` | `Scale` + optional arc ring |
| `MultiClickable` / `DoubleClickable` | `press/multiclick.h` | Stateless decorator + `ViewAdaptor` | `MultiTapGesture` (`DoubleClickable` delegates `count = 2`) | Callback + optional Button semantics |
| `ContextMenuArea` / `SecondaryClickable` | `press/contextmenu.h` | Stateless decorator + `ViewAdaptor` | `ContextHitTarget` + `ContextMenuRequested` + optional touch long press | Window-position callback |
| `Ripple` | `press/ripple.h` | Retained modifier (`With`) | Up to 4 active presses per node | Expanding/fading clipped circles |
| `Hoverable` / `HoverLift` | `hover/hoverlift.h` | Headless builder / Wrapper | Per-scope `hovered` cell | `Offset` + `Scale` on hover |
| `HoverHighlight` | `hover/highlight.h` | Retained modifier (`With`) | `InteractionState` controllers | Hover/press/focus tint overlay |
| `Spotlight` | `hover/spotlight.h` | Retained modifier (`With`) | Hover position + fade controller | Radial glow behind content |
| `DragSpring` | `gesture/dragspring.h` | Wrapper + `ViewAdaptor` | Per-scope drag + offset cells | Follow tween, spring return |
| `InertialDrag` | `gesture/inertialdrag.h` | Wrapper + `ViewAdaptor` | Per-scope drag + projected offset | Velocity-projected settle + `on_settled` |
| `PanZoom` / `PinchZoom` | `gesture/transform.h` | Wrapper + `PanZoomController` | Pan, scale, rotation (`PinchZoom` disables pan/rotation) | `Scale`, `Rotation`, `Offset` |
| `SwipeActions` | `gesture/swipe.h` | Wrapper + `SwipeController` | `SwipeActionContext{close, progress}` + `on_open_changed` | Clipped trailing strip + `Offset` |
| `Dismissible` | `gesture/dismissible.h` | Wrapper + background builder | Axis drag + exit generation | `Offset` exit + `on_dismissed` |
| `PressDragHandle` | `gesture/pressdraghandle.h` | Stateless decorator + `ViewAdaptor` | `DragGesture` with `minimum_press_duration` | Lifecycle callbacks |
| `ResizeHandle` | `gesture/resizehandle.h` | Composed separator | Per-scope `pressed` cell | Incremental axis delta callback |
| `Reorderable` / `ReorderableGrid` | `reorder/` | Container (`handle_only` option) | Shared `ReorderContext` + `SlotGeometry` | Slot-anchored preview + keyboard/auto-scroll |
| `ReorderHandle` | `reorder/reorderhandle.h` | Environment-bound grip | Reads `ReorderSlotEnvironment` | Drives enclosing slot drag session |
| `Draggable<T>` / `DropZone<T>` / `FileDropZone` | `dragdrop/` | Wrapper + headless builder | Per-scope `dragging` / `hovered` | Source opacity fade / hover builder or border |
| `HoverCard` | `overlay/hovercard.h` | Wrapper + `HoverCardController` | Anchored `PopupHandle` + dismiss sync | Delayed hover / touch popup |
| `ScrollProgress` / `ScrollEdgeFade` | `scroll/` | Retained modifier (`With`) | Connected `ScrollController` | Proportional thumb / edge gradients |
| `KeyboardShortcut` / `SemanticActions` | `accessibility/` | Decorator / Retained modifier | `KeyIntercept` / `BuildSemantics` | Chord callback / standard a11y action routing |
| `Pulsing` / `Shimmer` / `Fireworks` / `RollingNumber` / `Pop` / `Shake` / `RevealOnTap` | `visual/` | Wrapper / Modifier / Hook | Stateless, value-driven, or Pimpl trigger | Looping, particle, digit-strip, or burst motion |

## Conventions

- **Uniform parameter order**: `(View content, [required domain arguments], callback, Style style = {})`. Visual-only
  overloads `(View content, Style style = {})` coexist unambiguously via C++20 `std::invocable` constraints, avoiding
  empty `{}` placeholders at call sites.
- **Value equality everywhere**: Every `*Style` struct, retained modifier, trigger, and controller implements
  `operator==` so HuxerUI skips redundant updates and paint invalidations.
- **Primary-button press filtering**: `InteractiveSurface` and `Pressable` track pointer press state strictly for
  `PointerButton::Primary` (clearing on `Up` or any `Cancel`), preventing secondary clicks from triggering press scale.
- **Non-destructive hit-test participation**: `ContextMenuArea` uses an internal `HitTest`-only `NodeExtension` rather
  than binding a no-op `ViewEvents::Pointer` handler, so sibling pointer handlers on the same `View` are never
  overwritten.
