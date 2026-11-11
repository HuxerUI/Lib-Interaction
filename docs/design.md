# Lib-Interaction design

This document describes the widget catalog, the motion conventions shared by all widgets, and how each widget maps onto HuxerUI's animation and interaction layers.

## Positioning

Lib-Interaction extends HuxerUI with interactive animation widgets: views whose presentation responds to pointer presence and press activity. The library owns no rendering backend and no timing source; every animation flows through the framework's animation system:

- Declarative widgets project `Animated<T>` targets onto the built-in `Scale`, `Offset`, and `Opacity` presentation modifiers. Target changes animate from the current visible value without recomposing the content.
- Retained widgets (Ripple) drive a `MotionController` from `NodeExtension::OnFrame` and record paint commands in the extension's paint phases, with `InvalidatePaint` scoped to the foreground sequence.

The layering matches the framework's animation design: `TimingCurve`/`AnimationSpec` describe motion, controllers and animated modifiers own retained progress, and components choose semantic motion on top.

## Widget catalog

| Widget | Form | Interaction input | Motion output | State |
| --- | --- | --- | --- | --- |
| `Pressable` | Function returning `View` | `ViewEvents::Hover`, `ViewEvents::Pointer`, `OnClick` | `Scale` around one pivot | Per-scope hover and press cells |
| `HoverLift` | Function returning `View` | `ViewEvents::Hover` | `Offset` and `Scale` | Per-scope hover cell |
| `Pulsing` | Function returning `View` | None (always active) | `Scale` and/or `Opacity` with unbounded reverse playback | None |
| `DragSpring` | Function returning `View` | `DragGesture` + `DragEvents` | `Offset` following the gesture, spring return | Per-scope drag and offset cells |
| `Shaken` + `UseShake` | Function returning `View` + hook | `ShakeTrigger::Trigger()` from handlers/tasks | `Offset` tweens over a decaying wobble sequence | Trigger-owned offset and run cells, `TaskScope` |
| `LongPressable` | Function returning `View` | `LongPressGesture` + `LongPressEvents` | `Scale` while the press is held | Per-scope hold cell |
| `DoubleClickable` | Function returning `View` | `MultiTapGesture` + `MultiTapEvents` | None (callback and cursor only) | Per-scope binding |
| `Popped` + `UsePop` | Function returning `View` + hook | `PopTrigger::Trigger()` from handlers/tasks | `Scale` out-and-back tweens | Trigger-owned scale and run cells, `TaskScope` |
| `Ripple` | Modifier applied with `With()` | `NodeExtension::OnInteraction`, `OnPointer`, `HitTest` | Painted circles in `PaintAboveContent` | Retained per node, one ripple per press |
| `Spotlight` | Modifier applied with `With()` | `NodeExtension::OnHover`, `HoverHitTest` | Pointer-following radial gradient in `PaintBehindContent` | Retained per node |
| `Shimmer` | Modifier applied with `With()` | None (always sweeping) | Restarting highlight band in `PaintAboveContent` | Retained per node |
| `Fireworks` | Modifier applied with `With()` | `FireworksBurst` ids delivered through modifier updates | Particle bursts in `PaintAboveContent` | Retained per node, bounded burst list |
| `Reorderable` | Function returning `View` | `DragGesture` + `DragEvents` per slot | Dragged item follows the pointer, displaced items ease aside | Per-scope session plus shared slot geometry recorded in `PrepareGeometry` |
| `ReorderableGrid` | Function returning `View` | `DragGesture` + `DragEvents` per cell | Dragged cell follows the pointer, displaced cells ease to the closest slot | Per-scope session plus shared cell geometry |
| `SwipeActions` | Function returning `View` | `DragGesture` + `DragEvents` | Content `Offset` follows the pointer; open/closed settles from distance and velocity | Per-scope offset and drag session |
| `ContextMenuArea` | Function returning `View` | `ContextMenuRequested`, `LongPressGesture` | None | None |
| `Draggable` | Function returning `View` | `DragSource` + `DragGesture` | Source opacity fade | Per-scope dragging cell |
| `DropZone<T>` | Function returning `View` | `DropTarget` + `DropEvents<T>` | Highlight border while hovered | Per-scope hovered cell |
| `FileDropZone` | Function returning `View` | `FileDropTarget` + `FileDropEvents` | Highlight border while hovered | Per-scope hovered cell |
| `PanZoom` | Function returning `View` | `TransformGesture` + `TransformEvents`, `ScrollInput` | `Offset`, `Scale`, `Rotation` | Per-scope pan/scale/rotation and active cells |
| `InertialDrag` | Function returning `View` | `DragGesture` + `DragEvents` | `Offset` follows drag; release settles at a projected target | Per-scope offset and dragging cell |
| `ScrollProgress` | Modifier applied with `With()` | `NodeExtension::OnScrollActivity` + `ScrollController::Metrics` | Track and thumb in `PaintAboveContent` | Retained per node, connected controller |
| `ScrollEdgeFade` | Modifier applied with `With()` | `NodeExtension::OnScrollActivity` + `ScrollController::Metrics` | Edge gradients in `PaintAboveContent` | Retained per node, connected controller |
| `RollingNumber` | Function returning `View` | None (value-driven) | Clipped digit-column `Offset` targets | Per-scope digit strips |
| `SemanticActions` | Modifier applied with `With()` | `NodeExtension::BuildSemantics`, `OnSemanticAction` | None | Retained per node |
| `KeyboardShortcut` | Function returning `View` | `ViewEvents::KeyIntercept` | None | None |
| `MultiClickable` | Function returning `View` | `MultiTapGesture` + `MultiTapEvents` | None (callback/cursor only) | Per-scope binding |
| `SecondaryClickable` | Function returning `View` | `ViewEvents::Pointer`, `ContextMenuRequested` | None | None |
| `PressDragHandle` | Function returning `View` | `DragGesture` with `minimum_press_duration` | None (callbacks only) | None |
| `Dismissible` | Function returning `View` | `DragGesture` + `DragEvents` | `Offset` follows drag; exits past threshold | Per-scope offset, drag, and dismissal generation |
| `HoverCard` | Function returning `View` | `ViewEvents::Hover`, `LongPressGesture` | Popup presentation | Per-scope popup layer and generation |
| `UseRevealOnTap` | Composition hook | `SceneTransitionHandle::RunFromCurrentInteraction` | Circular reveal | Composition-scoped handle |
| `PinchZoom` | Function returning `View` | `TransformGesture` + `TransformEvents` | `Scale` following the gesture's spread changes | Per-scope zoom and gesture cells |
| `HoverHighlight` | Modifier applied with `With()` | `NodeExtension::OnInteraction`, `HitTest` | Hover/press/focus tint in `PaintAboveContent` | Retained per node |

### Pressable

`Pressable(content, style, on_click)` composes content with one animated `Scale` modifier. The effective target is `1.0` at rest, `style.hovered_scale` while hovered, and `style.pressed_scale` while pressed. The body runs inside an explicit `Scope`, so the widget does not require composable code generation in consumer targets. Validation happens at the call site, before the scope is created.

Pointer tracking accepts down, up, and cancel phases for the primary button. The hover binding treats every non-leave event as presence. Because a disabled node does not receive input, a press that is interrupted by becoming disabled relies on the framework delivering a cancel phase; this limitation is recorded in [open-questions.md](open-questions.md). The style carries a cursor kind applied through the framework's `PointerCursor` modifier when it is not `PointerCursorKind::Default`; the default shows the hand cursor over the composed surface.

When `on_click` is provided, the surface publishes a Button semantic role. When `keyboard_focusable` is set, the widget attaches the framework's `Focusable` modifier: the window-level keyboard activation handles Enter (immediate) and Space (press-then-release) activation, and the theme's focus-visible ring paints automatically while the surface holds keyboard focus. A `KeyIntercept` binding observes Space on the focused surface and applies `pressed_scale` while the key is held; `FocusChanged` clears that state when focus leaves, so the framework can cancel a keyboard activation without leaving the surface visually pressed.

### HoverLift

`HoverLift(content, style)` composes content with independent animated `Offset` and `Scale` modifiers that share one hover state and one `AnimationSpec`. Touch-only hosts without hover-capable presence keep the rest state.

### Pulsing

`Pulsing(content, style)` attaches unbounded reverse playback (`AnimationPlayback{.iterations = std::nullopt, .repeat_mode = RepeatMode::Reverse}`) to declarative targets: scale oscillates between `1.0` and `1.0 + scale_amplitude`, and opacity between `1.0` and `minimum_opacity`. At least one channel must participate. Reduced motion resolves both to their targets, which is the correct steady state for an attention pulse.

### DragSpring

`DragSpring(content, style)` attaches a `DragGesture` (optionally axis-constrained) and one animated `Offset` modifier. While the drag runs, the target tracks the gesture translation with a short linear tween; the widget swaps the `Animated` motion description at the same time the target returns to the origin on end or cancel, so the same modifier springs back with `DragSpringStyle::return_motion`. The cursor follows the drag state through one `PointerCursor` modifier: `DragSpringStyle::cursor` at rest (grab by default) and grabbing while the drag is owned. Terminal drag velocity is not handed to the spring in this version; the declarative `Animated` target form has no velocity input, so a velocity handoff needs a retained executor or framework support.

### Shake

`UseShake(style)` is a composition hook returning a copyable `ShakeTrigger`; `Shaken(content, trigger)` composes one animated `Offset` that reads the trigger's offset cell. `Trigger()` writes the first peak into the cell, launches a `TaskScope` task, and walks a decaying sequence of alternating targets paced by `Delay`, guarding every write with a run identifier so a newer trigger supersedes an older sequence. `ShakeStyle::axis` picks the travel direction — horizontal by default, vertical when set. Every sequence ends at rest, which keeps reduced motion correct: tweens snap between poses and the final pose is the rest position. `TaskScope::Launch` throws during composition, so triggering is only legal from handlers and tasks; the binding states live in the caller's composition scope and are created by `UseShake` inline in the header, matching the SDK's own hook precedent.

### LongPressable

`LongPressable(content, style, on_long_press)` composes content with one animated `Scale` modifier and the framework's `LongPressGesture`. The gesture wins the pointer sequence when the hold duration elapses within the movement tolerance; the callback fires once at recognition, the scale target drops to `style.held_scale` while the press is owned, and Ended or Canceled restores rest. Recognition follows the platform `GestureSettings` unless the style overrides the duration or movement. The style also carries a cursor kind applied through the `PointerCursor` modifier when it is not `PointerCursorKind::Default`.

With `show_progress`, the widget attaches an internal retained modifier that paints a circular ring above the content: one `MotionController` fills the arc across the resolved hold duration — `minimum_duration` when set, otherwise the framework's 0.5 s default — and the same resolved value is applied to the gesture so ring and recognition stay synchronized. The ring drains with a short tween when the pointer leaves early. The extension observes the pointer sequence through `PointerResult::Observe` and never consumes events.

### DoubleClickable

`DoubleClickable(content, style, on_double_click)` attaches the platform `MultiTapGesture` with a count of two and forwards `MultiTapEvents::Recognized` to the callback together with the final tap event, so position-sensitive reactions such as a `Fireworks` burst at the pointer stay node-local. Recognition explicitly does not replace ordinary Click handling, so OnClick and sibling gestures stay composable on the same view. There is no built-in press presentation; compose `Pressable` at the call site when the double-click target also needs hover and press feedback. The default cursor is the hand.

### Fireworks

`Fireworks` is a retained, paint-only modifier driven by its modifier value: the caller composes `FireworksBurst` requests from application state, and when the incoming id differs from the last delivered one, the extension spawns a radial particle burst at the request's node-local position. Each burst integrates one `MotionController` over `FireworksStyle::duration`; particles derive deterministically from per-particle random parameters — direction, distance, radius, stagger, and palette color — so positions and the gravity droop are pure functions of progress. Bursts are bounded to three; the extension requests frames only while bursts are active, so an idle modifier never keeps the frame loop awake, and reduced motion resolves bursts immediately to their settled, invisible state.

### Pop

`UsePop(style)` mirrors `UseShake`: a composition hook returning a copyable `PopTrigger`, with `Popped(content, trigger)` composing one animated `Scale` that reads the trigger's scale cell. `Trigger()` writes the peak `1.0 + amplitude` into the cell and launches a `TaskScope` task that restores rest after `PopStyle::duration`, guarded by a run identifier so a newer trigger supersedes an older sequence. Every sequence ends at rest, which keeps reduced motion correct: tweens snap between poses and the final pose is the rest scale.

### Reorderable

`Reorderable(items, style, on_reordered)` composes items in a row or column, selected by `style.axis`, with drag-to-reorder following the slot-recording pattern proven by Clash-Flux's profile grid. An internal retained modifier overrides `PrepareGeometry` to record each slot's undragged window-space bounds into a shared geometry map, so hit testing and preview targeting stay anchored to layout positions while displaced items animate. While a drag runs the layout stays in the original order — the dragged item follows the pointer with `follow_motion` from its own slot, the items between the source and the pointer's closest slot (point-to-rect distance, gap included) ease one position aside with `settle_motion`, and the closest slot draws the target highlight. Dropping fires `on_reordered(from, to)` once and resets the session; the model reorders at recomposition, which keeps the pointer session alive because no node is re-keyed mid-drag.

### SwipeActions

`SwipeActions(content, actions, style)` composes an internal custom layout: the action strip is declared first, measured to the content height and the configured action extent, and placed behind the content at the trailing edge; the content is declared second and translated left by a `DragGesture`. The root clips descendants, so the action strip is revealed only as the content moves and remains clickable once exposed. An action builder receives a copyable `close` callback, letting action buttons dismiss the strip after performing their command. A release opens the strip when the exposed fraction reaches `activation_ratio` or the leftward velocity reaches `minimum_velocity`; otherwise it settles closed. A canceled drag always closes.

### Draggable and typed drop zones

`Draggable(content, payload, style, preview, gesture)` attaches the framework's `DragSource` with the exact payload type. The modifier supplies the typed drag session and optional preview; the wrapper adds a retained `Opacity` target that fades the source while `DragSourceEvents::Started` is active and restores it on `Ended` or `Canceled`. A non-default cursor is attached through `PointerCursor`.

`DropZone<T>(content, on_drop, style)` attaches `DropTarget::Accepts<T>()` and listens for the typed `DropEvents<T>` lifecycle. Entered and Moved set a hovered cell, Exited clears it, and Dropped clears it before forwarding the borrowed payload. The border is always present in the modifier value: the highlight color and width are selected from the hovered state, with a zero-width transparent border at rest.

`FileDropZone(content, options, on_dropped, style)` is the external-file counterpart. `FileDropTarget::Accepts(options)` is constructed before the composition scope so malformed filters throw at the call site. Entered and Moved highlight, Exited and Failed clear, and Dropped forwards the accepted file batch. Neither zone consumes pointer events or owns transferred data.

### ContextMenuArea

`ContextMenuArea(content, on_request, style)` binds the framework's `ContextMenuRequested` event, which already covers unchorded secondary taps, the Context Menu key, and Shift+F10. When touch long press is enabled it also attaches `LongPressGesture`; touch recognition forwards the event's window position because the framework intentionally does not convert touch long presses into context-menu events. The wrapper does not consume ordinary clicks or gestures and imposes no layout.

Focused slots are `Focusable`; Alt+arrow moves the focused item one slot along the configured axis and calls `on_reordered` without starting a pointer drag. The root publishes a List semantic role and each slot a ListItem with its source index. When `auto_scroll_edge` and a connected `scroll_controller` are present, a drag update near the viewport edge requests one `auto_scroll_step` of scroll, so continuous pointer motion keeps revealing more of a long list. Auto-scroll is intentionally per drag update rather than a retained timer.

### ReorderableGrid

`ReorderableGrid(items, style, on_reordered)` uses `Flow` with fixed `Frame` cells. A `GridSlotBounds` extension records each cell's window bounds during geometry preparation; a linear `DisplacedIndex` shifts cells between source and target, which works for wrapping rows as well as columns. Dragging follows the pointer, dropping reports linear indices, and the target slot draws the same outline treatment as `Reorderable`. This is the 2D layout counterpart to the 1D reorder widget; it deliberately keeps layout order until drop so cells do not repack mid-gesture.

### PinchZoom

`PinchZoom(content, style)` attaches the framework's `TransformGesture` and one animated `Scale` modifier. Scale updates multiply the current zoom by the gesture's incremental spread change, clamped to the configured range; the zoom persists at the released value unless `PinchZoomStyle::spring_back` is set, in which case Ended swaps the motion description for the return spring while the target resets to rest — the same motion-swap trick `DragSpring` uses. On wheel and trackpad hosts, `ViewEvents::ScrollInput` with the control modifier held drives the same clamped zoom multiplicatively from the scroll delta and consumes the event only while the modifier is held, so surrounding scroll containers keep working; the fallback exists because desktop backends deliver no transform gestures. Pan and rotation updates are ignored in this version because the declarative presentation set has no rotation modifier; a rotation-capable version needs a framework rotate presentation modifier.

### PanZoom

`PanZoom(content, style)` accumulates `TransformEvent` deltas: `pan` is added to the current offset, `scale` is multiplied and clamped to the configured range, and `rotation` is converted from radians to the degrees consumed by the `Rotation` modifier. Its presentation chain applies `Scale`, optional `Rotation`, and `Offset` around the configured origin. The terminal deltas remain application-visible until a later gesture; `spring_back` resets all three on `Ended` or `Canceled` with `return_motion`. Ctrl+wheel input multiplies zoom by an exponential delta and consumes the event; unmodified scroll passes through. `pan_bounds` clamps accumulated pan per axis, `rotation_snap` rounds rotation on release, and `double_tap_reset` restores rest through the shared MultiTapGesture. Unlike `PinchZoom`, which only accumulates scale, `PanZoom` owns the complete two-pointer transform and has no inertia.

### InertialDrag

`InertialDrag(content, style)` uses `DragEvent::translation` while the drag owns the pointer and, on `Ended`, computes `translation + velocity * projection_time` on each axis, clamps it to `maximum_projection`, and animates there with `settle_motion`. This is a velocity-projected target, not a velocity-continuous retained simulation; the declarative `AnimateTo` path has no initial-velocity input. Cancellation returns to rest. `DragSpring` remains the spring-back alternative; this widget is for surfaces that should keep the projected displacement.

### ScrollProgress and ScrollEdgeFade

Both modifiers take a `ScrollController` that must be bound to the same `ScrollView`. They listen to `NodeExtension::OnScrollActivity` for invalidation and read `ScrollController::Metrics()` during paint, so they never recompose for scroll movement. `ScrollProgress` hides when the controller is disconnected or `maximum_offset` is zero, draws a track and a proportional thumb along the metrics axis, and clamps the thumb to the track. `ScrollEdgeFade` draws a start gradient only after the offset leaves zero and an end gradient only while offset remains below `maximum_offset`; the configured color should match the surface behind the scrollable content. Neither modifier mutates scroll state.

### RollingNumber

`RollingNumber(value, style)` builds one clipped digit window per character. `UseTextMeasurer` supplies digit advances and line height, so callers do not provide frame metrics. Each window contains a zero-through-nine column whose `Offset` target is `-digit * height`; a value change retargets the existing animation, which eases from the currently visible digit. A negative value gets a static sign cell, and `minimum_digits` pads the magnitude with leading zeros. The widget is value-driven and has no scroll or gesture state.

### SemanticActions, KeyboardShortcut, and repeated input

`SemanticActions` is a retained modifier that merges owner `Semantics`, advertises each standard `SemanticActionKind` in its mask through `SemanticBuilder::AddAction`, and routes `OnSemanticAction` back to the supplied callback. The Custom action is intentionally rejected because it needs an extension-local custom action ID; use a dedicated NodeExtension for that case.

`KeyboardShortcut` attaches `ViewEvents::KeyIntercept` with an exact `KeyChord`. On the focus-scope root it acts as an application shortcut; around a focused control it acts locally. The initial non-repeat Down invokes the callback and optionally consumes the event.

`MultiClickable` wraps `MultiTapGesture` with a configurable count of two or more; the existing `DoubleClickable` remains the fixed two-tap convenience. `SecondaryClickable` adds a raw pointer binding so an otherwise inert surface participates in hit testing, then routes `ContextMenuRequested`; it does not synthesize touch long press.

### PressDragHandle and Dismissible

`PressDragHandle` attaches a `DragGesture` with `minimum_press_duration` and forwards Started/Changed/Ended/Canceled to caller callbacks. It owns no offset or model mutation, so a reorder or drag surface can treat the handle as its only drag origin.

`Dismissible` places content over a `Background` inside a clipped `Stack`, translates the content with `Offset`, and dismisses when the axis displacement reaches `dismiss_threshold` or the matching-axis velocity reaches `minimum_velocity`. A dismissed item travels to `exit_distance` with `exit_motion`; a scoped `TaskScope` waits `exit_duration` before invoking `on_dismissed`, guarded by a generation so a newer drag or cancel suppresses the stale callback. Below threshold and on cancellation the content returns to rest.

### HoverCard

`HoverCard` uses a scoped `PopupHandle` anchor. Hover Enter schedules a `TaskScope` delay and then updates or shows the anchored popup; Leave cancels a pending show and schedules dismissal after `hide_delay`, guarded by a generation token. Touch long press shows immediately and its Ended/Canceled lifecycle schedules the same grace-period hide. Repeated enters call `PopupHandle::Update` while the layer is live, so another popup is not stacked. The anchor remains ordinary content and keeps its other gestures.

### Default semantics

`Pressable`, `LongPressable`, and `DoubleClickable` attach `Semantics{.role = Button}` only when they have an activation callback, so a purely visual wrapper does not claim a command. `RollingNumber` publishes `Text` with the numeric label and excludes its ten digit windows from descendant semantics. Callers can override any field by applying `Semantics` after the widget.

### RevealOnTap

`UseRevealOnTap` returns a copyable handle for `SceneTransitionHandle::RunFromCurrentInteraction` with a default `CircularRevealTransition`. Call `Run` synchronously from a click, pointer, keyboard, or semantic callback; it inherits the exact interaction origin and reduced-motion policy. Asynchronous continuations must capture explicit geometry and use a SceneTransitionHandle directly.

### Spotlight

`Spotlight` is a retained modifier using the explicit descriptor form. The extension overrides `HoverHitTest` to receive hover entry, movement, and departure, stores the latest node-local pointer position, and repaints only the sequence behind node content: one `RadialGradient`-filled rect whose normalized center is the pointer position and whose radius fraction derives from `SpotlightStyle::diameter` over the node size. Presence is one `MotionController` fading in on enter and out on leave; reduced motion resolves it immediately. No application state and no recomposition are involved at any point.

### HoverHighlight

`HoverHighlight` is a retained modifier driven by `NodeExtension::OnInteraction`: the hover/press controller target is `2.0` while the node reports pressed, `1.0` while hovered, and `0.0` at rest or disabled. A second controller fades `focus_color` while `focus_visible` is set; its transparent default leaves the theme focus ring as the only focus treatment. The overlay blends from transparent to `hover_color` and from there toward `pressed_color`, painted above content and clipped to the node bounds and style corner radii. The extension also overrides `HitTest` and `HoverHitTest` within the node bounds so inert views (no events of their own) still report interaction state; the modifier never consumes pointer events.

### Ripple

`Ripple` is a retained modifier using the framework's explicit descriptor form (`static const detail::ModifierDescriptor& Descriptor()`), so the extension class can stay out of the public header. The extension:

- Returns `true` from `HitTest` within node bounds so the node participates in pointer routing.
- Starts a ripple on each `InteractionEvent::Press` at the node-local pointer position, or at the node center when `RippleStyle::center_origin` is set or the press came from the keyboard. `OnPointer` still returns `PointerResult::Observe` for an inert view so it becomes the interaction target without consuming click handling. Each active press owns one independent ripple, bounded to four simultaneous circles.
- Advances one `MotionController` per active ripple per frame, invalidating only the foreground paint sequence when a value changes.
- Paints expanding, fading circles clipped to the node bounds and the style's corner radii. Radius covers the distance to the farthest corner; the alpha envelope is `color.alpha * maximum_opacity * (1 - progress)`.
- Resolves a pointer's ripple immediately on cancel and under reduced motion.

### Shimmer

`Shimmer` is a retained, paint-only modifier for placeholder content. The extension starts one `MotionController` sweep with unbounded `RepeatMode::Restart` playback and repaints a horizontal highlight band in `PaintAboveContent`: a `LinearGradient` whose stops are recomputed each frame from the sweep progress, clipped to the node bounds and the style's corner radii. Like the declarative looping widgets, the sweep rides whatever frame traffic the host already produces — its `OnFrame` never requests frames — so an always-mounted shimmer never keeps an otherwise idle frame loop awake; the same property lets the test harness settle. Reduced motion resolves the controller to its target, which parks the band outside the node and leaves the content plain.

## Validation and error policy

Following the interface conventions of the other Lib projects, invalid caller configuration throws `std::invalid_argument` from the composition call. All style structs provide value equality so the framework can skip no-op modifier updates.

## Conventions

- Types and functions use PascalCase; fields use snake_case.
- Widgets impose no intrinsic layout size; callers supply bounds through a parent or `Frame`.
- Options structs describe one responsibility, document ranges and defaults in the header, and validate on entry.
- The library never requires composable code generation; composition-bound work runs inside explicit `Scope` factories.
- `huxerui_add_library` still routes library sources through the composable-source scan, so widget sources must remain valid transformer input: unmarked files pass through unchanged, but lambdas inside them must respect the framework's rvalue-qualified fluent API (`With`/`On`/`Key`/`OnClick` consume `View&&`, so chain them on mutable locals, not on const captures).
- Accessibility labels remain the caller's responsibility through `Semantics`, matching the other Lib projects.

## Roadmap candidates

Not implemented in this version; see [open-questions.md](open-questions.md) for the pending decisions:

- `Expandable`/`Collapse` with real height animation (requires an animated-size or clip-height primitive in the SDK).
- `Marquee` (the text rail still needs measured travel or a caller-supplied travel distance; `ClipChildren` provides the clipping primitive).
- `Magnetic` pointer-following content (requires a composition-time node size hook or normalized hover positions; HoverEvent reports node-local coordinates only).
- Shake and attention gestures driven by `MotionController` with velocity handoff.
- Shared `InteractionCallbacks` type for widgets that need to mirror state into application models.
