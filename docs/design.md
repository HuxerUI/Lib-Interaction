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
   - **Headless builders** (`InteractiveSurface`, `HoldRepeat`, `Hoverable`, `Copyable`, `Popover` trigger builder,
     `PredictiveBack` builder, `StickyHeaderArea` builder, `ScrollParallax` builder, `SnapDrag` builder, `SwipeDeck`
     builder, and builder overloads of `DropZone<T>`, `FileDropZone`, `Dismissible`) run the
     input/focus/semantics/repeat/back/sticky/parallax/snap/deck state machine and pass live state snapshots
     (`InteractionSnapshot`, `HoldRepeatSnapshot`, `StickyHeaderSnapshot`, `ScrollParallaxSnapshot`, `hovered`, `copied`,
     `PredictiveBackSnapshot`, `SnapDragSnapshot`, `SwipeDeckCardContext`, `progress`) to caller-supplied view
     builders.
   - **Controllers** (`HoldRepeatController`, `SwipeController`, `SwipeDeckController`, `PanZoomController`,
     `HoverCardController`, `PopoverController`, `ScrollHideController`, `ScrollSpyController`, `StickyHeaderController`,
     `SnapDragController`, `RovingFocusController`, `PredictiveBackController`, `MarqueeSelectController`) use
     `std::shared_ptr` state handles remembered via `Use*Controller()`, enabling external state reads and programmatic
     actions while keeping default-constructed handles runtime-free.
   - **Environment-bridged grips & pipelines**: `ReorderHandle`, `RovingFocusItem`, `MarqueeSelectItem`,
     `ScrollSpySection`, and `StickyHeaderSection` connect to enclosing containers via `Environment`
     (`ProvideEnvironment` / `UseEnvironment`), and `pipe.h` provides left-to-right `operator|` composition via
     `ViewAdaptor`.
3. **Layer 2 — Styled presets, Retained modifiers, and `visual/`**:
   - **Input-driven presets** (`Pressable`, `HoldRepeat`, `Copyable`, `HoverLift`, `HoverShadow`, `Magnetic`,
     `LongPressable`, `DragSpring`, `InertialDrag`, `SnapDrag`, `MarqueeSelectArea`, `PanZoom`, `PinchZoom`,
     `SwipeActions`, `SwipeDeck`, `Dismissible`, `Reorderable`, `ReorderableGrid`, `Draggable`, `DropZone`,
     `FileDropZone`, `HoverCard`, `Popover`, `ScrollParallax`, `ScrollSpyArea`, `StickyHeaderList`) build default motion
     and chrome on top of Layer 1.
   - **Retained modifiers** (`Ripple`, `HoldProgressRing`, `Spotlight`, `HoverHighlight`, `ScrollProgress`,
     `ScrollEdgeFade`, `NestedScrollConnection`, `SemanticActions`) attach `NodeExtension` behavior through
     `View::With()` and implement `operator==` so unchanged recompositions skip extension updates.
   - **Visual/motion domain (`visual/`)** isolates non-interactive and value-driven presentation effects (`Expandable`,
     `Appear`, `UseSharedMorph` / `MorphScope` / `MorphElement` / `MorphBounds`, `Pulsing`, `Shimmer`, `Fireworks`,
     `RollingNumber`, `UsePop`/`Popped`, `UseShake`/`Shaken`, `UseRevealOnTap`) from input recognizers.

## Catalog

| Component | Header family | Form | State / Control | Motion / Output |
| --- | --- | --- | --- | --- |
| `InteractiveSurface` | `press/surface.h` | Headless builder | `InteractionSnapshot{hovered, pressed, focused}` | Caller-defined |
| `Pressable` | `press/pressable.h` | Wrapper + `ViewAdaptor` | Built on `InteractiveSurface` (primary button + Space) | `Scale` around `origin` |
| `HoldRepeat` | `press/holdrepeat.h` | Headless / Wrapper + `HoldRepeatController` | `HoldRepeatSnapshot` + `TaskScope::Delay` accelerating loop | `Scale` + `void()` / `void(step)` repeat ticks |
| `Copyable` | `press/copyable.h` | Headless / Wrapper + `ViewAdaptor` | `Clipboard::WriteText` + `TaskScope::Delay` reset | Timed `copied` feedback + `on_copied` |
| `LongPressable` + `HoldProgressRing` | `press/longpress.h` | Wrapper + Modifier | Per-scope `holding` + `on_holding_changed` | `Scale` + optional arc ring |
| `MultiClickable` / `DoubleClickable` | `press/multiclick.h` | Stateless decorator + `ViewAdaptor` | `MultiTapGesture` (`DoubleClickable` delegates `count = 2`) | Callback + optional Button semantics |
| `ContextMenuArea` / `SecondaryClickable` | `press/contextmenu.h` | Stateless decorator + `ViewAdaptor` | `ContextHitTarget` + `ContextMenuRequested` + optional touch long press | Window-position callback |
| `Ripple` | `press/ripple.h` | Retained modifier (`With`) | Up to 4 active presses per node | Expanding/fading clipped circles |
| `Hoverable` / `HoverLift` | `hover/hoverlift.h` | Headless builder / Wrapper | Per-scope `hovered` cell | `Offset` + `Scale` on hover |
| `Magnetic` | `hover/magnetic.h` | Wrapper + `ViewAdaptor` | Outer stationary probe `Bounds()` + `OnHover` | Geometry-scaled `Offset` + `Scale` |
| `HoverShadow` | `hover/hovershadow.h` | Wrapper + `ViewAdaptor` | Pointer position + fade/axis controllers | Pointer-following outer shadow/glow + radial rim highlight |
| `HoverHighlight` | `hover/highlight.h` | Retained modifier (`With`) | `InteractionState` controllers | Hover/press/focus tint overlay |
| `Spotlight` | `hover/spotlight.h` | Retained modifier (`With`) | Hover position + fade controller | Radial glow behind content |
| `DragSpring` | `gesture/dragspring.h` | Wrapper + `ViewAdaptor` | Per-scope drag + offset cells | Follow tween, spring return |
| `InertialDrag` | `gesture/inertialdrag.h` | Wrapper + `ViewAdaptor` | Per-scope drag + projected offset | Velocity-projected settle + `on_settled` |
| `SnapDrag` | `gesture/snapdrag.h` | Builder / Wrapper + `SnapDragController` | Stage/item bounds + `MotionController::Seek` velocity handoff | Corner / edge / custom anchor spring snap + drag scale |
| `MarqueeSelectArea` / `MarqueeSelectItem` | `gesture/marqueeselect.h` | Container + Item + `MarqueeSelectController` | `Environment` + `PrepareGeometry` bounds + `DragGesture` | Rubber-band box + multi-index selection |
| `PanZoom` / `PinchZoom` | `gesture/transform.h` | Wrapper + `PanZoomController` | Pan, scale, rotation (`PinchZoom` disables pan/rotation) | `Scale`, `Rotation`, `Offset` |
| `SwipeActions` | `gesture/swipe.h` | Wrapper + `SwipeController` | `SwipeActionContext{close, progress}` + `on_open_changed` | Clipped trailing strip + `Offset` |
| `SwipeDeck` | `gesture/swipedeck.h` | Builder / Stacked cards + `SwipeDeckController` | `SwipeDeckCardContext` + velocity-continuous `MotionController::Seek` + rewind stack | Card rotation tilt + depth promotion (`Scale`, `Offset`, `Opacity`) + programmatic `Swipe()`/`Rewind()` |
| `Dismissible` | `gesture/dismissible.h` | Wrapper + background builder | Axis drag + exit generation | `Offset` exit + `on_dismissed` |
| `PressDragHandle` | `gesture/pressdraghandle.h` | Stateless decorator + `ViewAdaptor` | `DragGesture` with `minimum_press_duration` | Lifecycle callbacks |
| `ResizeHandle` | `gesture/resizehandle.h` | Composed separator | Per-scope `pressed` cell | Incremental axis delta callback |
| `Reorderable` / `ReorderableGrid` | `reorder/` | Container (`handle_only` option) | Shared `ReorderContext` + `SlotGeometry` | Slot-anchored preview + keyboard/auto-scroll |
| `ReorderHandle` | `reorder/reorderhandle.h` | Environment-bound grip | Reads `ReorderSlotEnvironment` | Drives enclosing slot drag session |
| `Draggable<T>` / `DropZone<T>` / `FileDropZone` | `dragdrop/` | Wrapper + headless builder | Per-scope `dragging` / `hovered` + predicate filter | Source opacity fade / hover builder or border |
| `HoverCard` | `overlay/hovercard.h` | Wrapper + `HoverCardController` | Anchored `PopupHandle` + dismiss sync | Delayed hover / touch popup |
| `Popover` | `overlay/popover.h` | Headless / Wrapper + `PopoverController` | Click toggle + `Lifecycle` dismiss sync | Anchored focus-trapped popup |
| `ScrollProgress` / `ScrollEdgeFade` | `scroll/` | Retained modifier (`With`) | Connected `ScrollController` | Proportional thumb / edge gradients |
| `ScrollParallaxArea` / `ScrollParallax` | `scroll/parallax.h` | Viewport + Builder / Wrapper + `ViewAdaptor` | `Environment` + `PrepareGeometry` / `ScrollController::Metrics()` + `reduced_motion` | Viewport-relative `Offset`, peak `Scale`, edge `Opacity`, optional `ClipChildren` |
| `NestedScrollConnection` / `ScrollHideController` | `scroll/nestedscroll.h` | Modifier + `UseScrollHideController` | Pre/post scroll & fling hooks | Signed clamped deltas + hidden offset state |
| `ScrollSpyArea` / `ScrollSpySection` | `scroll/scrollspy.h` | Container + Section + `ScrollSpyController` | `Environment` + `PrepareGeometry` (`PresentationBounds`) + `TaskScope::Post` | Active leading section tracking + `Smooth` / `Instant` `ScrollToSection()` (`scroll_motion`) |
| `StickyHeaderArea` / `StickyHeaderSection` / `StickyHeaderList` | `scroll/stickyheader.h` | Overlay stage + Section + List + `StickyHeaderController` | `StickyHeaderStageLayout` + `Environment` + `PrepareGeometry` + `ScrollController::Metrics()` | Zero-lag pinned section overlay, collision push-up offset, `Smooth` / `Instant` `ScrollToSection()` |
| `KeyboardShortcut` / `ShortcutMap` / `SemanticActions` | `accessibility/` | Decorator / Retained modifier | Single or multi-entry `KeyIntercept` / `BuildSemantics` | Chord callbacks / standard a11y routing |
| `BackHandler` / `PredictiveBack` | `accessibility/backhandler.h` | Decorator / Builder + `PredictiveBackController` | `NodeExtension::OnBack` (`Begin`/`Update`/`Cancel`/`Commit`) | Back interception + predictive offset/scale preview |
| `RovingFocusGroup` / `RovingFocusItem` | `accessibility/rovingfocus.h` | Group + Item + `RovingFocusController` | `Environment` + `FocusRequester` + `OnFocusChanged` | Arrow / Home / End traversal skipping disabled |
| `Expandable` | `visual/expandable.h` | Layout wrapper + `ViewAdaptor` | `MotionController` -> `ExpandableLayout` | Axis collapse (`0` size + child excluded closed) |
| `Appear` | `visual/appear.h` | Wrapper + `ViewAdaptor` | 0-dep `Lifecycle` mount trigger + `visible` | Synchronized `Transition` (`Opacity`/`Offset`/`Scale`) |
| `SharedMorph` (`UseSharedMorph`) | `visual/sharedmorph.h` | Handle + `MorphScope`/`MorphElement`/`MorphBounds` | `SharedTransitionHandle` | Local shared-element / bounds morphing |
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
