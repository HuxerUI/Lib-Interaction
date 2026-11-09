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
| `Ripple` | Modifier applied with `With()` | `NodeExtension::OnPointer`, `HitTest` | Painted circles in `PaintAboveContent` | Retained per node, one ripple per active pointer |
| `Spotlight` | Modifier applied with `With()` | `NodeExtension::OnHover`, `HoverHitTest` | Pointer-following radial gradient in `PaintBehindContent` | Retained per node |
| `Shimmer` | Modifier applied with `With()` | None (always sweeping) | Restarting highlight band in `PaintAboveContent` | Retained per node |
| `Fireworks` | Modifier applied with `With()` | `FireworksBurst` ids delivered through modifier updates | Particle bursts in `PaintAboveContent` | Retained per node, bounded burst list |
| `HoverHighlight` | Modifier applied with `With()` | `NodeExtension::OnInteraction`, `HitTest` | Interaction tint in `PaintAboveContent` | Retained per node |

### Pressable

`Pressable(content, style, on_click)` composes content with one animated `Scale` modifier. The effective target is `1.0` at rest, `style.hovered_scale` while hovered, and `style.pressed_scale` while pressed. The body runs inside an explicit `Scope`, so the widget does not require composable code generation in consumer targets. Validation happens at the call site, before the scope is created.

Pointer tracking accepts down, up, and cancel phases for the primary button. The hover binding treats every non-leave event as presence. Because a disabled node does not receive input, a press that is interrupted by becoming disabled relies on the framework delivering a cancel phase; this limitation is recorded in [open-questions.md](open-questions.md). The style carries a cursor kind applied through the framework's `PointerCursor` modifier when it is not `PointerCursorKind::Default`; the default shows the hand cursor over the composed surface.

When `on_click` is provided and `keyboard_focusable` is set, the widget attaches the framework's `Focusable` modifier: the window-level keyboard activation handles Enter (immediate) and Space (press-then-release) activation, and the theme's focus-visible ring paints automatically while the surface holds keyboard focus. Held-Space scale feedback is not included — `InteractionState` is not yet exposed to composition — and that gap is recorded in the roadmap.

### HoverLift

`HoverLift(content, style)` composes content with independent animated `Offset` and `Scale` modifiers that share one hover state and one `AnimationSpec`. Touch-only hosts without hover-capable presence keep the rest state.

### Pulsing

`Pulsing(content, style)` attaches unbounded reverse playback (`AnimationPlayback{.iterations = std::nullopt, .repeat_mode = RepeatMode::Reverse}`) to declarative targets: scale oscillates between `1.0` and `1.0 + scale_amplitude`, and opacity between `1.0` and `minimum_opacity`. At least one channel must participate. Reduced motion resolves both to their targets, which is the correct steady state for an attention pulse.

### DragSpring

`DragSpring(content, style)` attaches a `DragGesture` (optionally axis-constrained) and one animated `Offset` modifier. While the drag runs, the target tracks the gesture translation with a short linear tween; the widget swaps the `Animated` motion description at the same time the target returns to the origin on end or cancel, so the same modifier springs back with `DragSpringStyle::return_motion`. The cursor follows the drag state through one `PointerCursor` modifier: `DragSpringStyle::cursor` at rest (grab by default) and grabbing while the drag is owned. Terminal drag velocity is not handed to the spring in this version; the declarative `Animated` target form has no velocity input, so a velocity handoff needs a retained executor or framework support.

### Shake

`UseShake(style)` is a composition hook returning a copyable `ShakeTrigger`; `Shaken(content, trigger)` composes one animated `Offset` that reads the trigger's offset cell. `Trigger()` writes the first peak into the cell, launches a `TaskScope` task, and walks a decaying sequence of alternating targets paced by `Delay`, guarding every write with a run identifier so a newer trigger supersedes an older sequence. Every sequence ends at rest, which keeps reduced motion correct: tweens snap between poses and the final pose is the rest position. `TaskScope::Launch` throws during composition, so triggering is only legal from handlers and tasks; the binding states live in the caller's composition scope and are created by `UseShake` inline in the header, matching the SDK's own hook precedent.

### LongPressable

`LongPressable(content, style, on_long_press)` composes content with one animated `Scale` modifier and the framework's `LongPressGesture`. The gesture wins the pointer sequence when the hold duration elapses within the movement tolerance; the callback fires once at recognition, the scale target drops to `style.held_scale` while the press is owned, and Ended or Canceled restores rest. Recognition follows the platform `GestureSettings` unless the style overrides the duration or movement. The style also carries a cursor kind applied through the `PointerCursor` modifier when it is not `PointerCursorKind::Default`.

With `show_progress`, the widget attaches an internal retained modifier that paints a circular ring above the content: one `MotionController` fills the arc across the resolved hold duration — `minimum_duration` when set, otherwise the framework's 0.5 s default — and the same resolved value is applied to the gesture so ring and recognition stay synchronized. The ring drains with a short tween when the pointer leaves early. The extension observes the pointer sequence through `PointerResult::Observe` and never consumes events.

### DoubleClickable

`DoubleClickable(content, style, on_double_click)` attaches the platform `MultiTapGesture` with a count of two and forwards `MultiTapEvents::Recognized` to the callback together with the final tap event, so position-sensitive reactions such as a `Fireworks` burst at the pointer stay node-local. Recognition explicitly does not replace ordinary Click handling, so OnClick and sibling gestures stay composable on the same view. There is no built-in press presentation; compose `Pressable` at the call site when the double-click target also needs hover and press feedback. The default cursor is the hand.

### Fireworks

`Fireworks` is a retained, paint-only modifier driven by its modifier value: the caller composes `FireworksBurst` requests from application state, and when the incoming id differs from the last delivered one, the extension spawns a radial particle burst at the request's node-local position. Each burst integrates one `MotionController` over `FireworksStyle::duration`; particles derive deterministically from per-particle random parameters — direction, distance, radius, stagger, and palette color — so positions and the gravity droop are pure functions of progress. Bursts are bounded to three; the extension requests frames only while bursts are active, so an idle modifier never keeps the frame loop awake, and reduced motion resolves bursts immediately to their settled, invisible state.

### Pop

`UsePop(style)` mirrors `UseShake`: a composition hook returning a copyable `PopTrigger`, with `Popped(content, trigger)` composing one animated `Scale` that reads the trigger's scale cell. `Trigger()` writes the peak `1.0 + amplitude` into the cell and launches a `TaskScope` task that restores rest after `PopStyle::duration`, guarded by a run identifier so a newer trigger supersedes an older sequence. Every sequence ends at rest, which keeps reduced motion correct: tweens snap between poses and the final pose is the rest scale.

### Spotlight

`Spotlight` is a retained modifier using the explicit descriptor form. The extension overrides `HoverHitTest` to receive hover entry, movement, and departure, stores the latest node-local pointer position, and repaints only the sequence behind node content: one `RadialGradient`-filled rect whose normalized center is the pointer position and whose radius fraction derives from `SpotlightStyle::diameter` over the node size. Presence is one `MotionController` fading in on enter and out on leave; reduced motion resolves it immediately. No application state and no recomposition are involved at any point.

### HoverHighlight

`HoverHighlight` is a retained modifier driven by `NodeExtension::OnInteraction`: the controller target is `2.0` while the node reports pressed, `1.0` while hovered, and `0.0` at rest or disabled. The overlay blends from transparent to `hover_color` and from there toward `pressed_color`, painted above content and clipped to the node bounds and style corner radii. The extension also overrides `HitTest` and `HoverHitTest` within the node bounds so inert views (no events of their own) still report interaction state; the modifier never consumes pointer events.

### Ripple

`Ripple` is a retained modifier using the framework's explicit descriptor form (`static const detail::ModifierDescriptor& Descriptor()`), so the extension class can stay out of the public header. The extension:

- Returns `true` from `HitTest` within node bounds so the node participates in pointer routing.
- Starts a ripple on primary-button down at the node-local position and returns `PointerResult::Observe`, leaving click and gesture handling untouched. Each active pointer owns one independent ripple, bounded to four simultaneous circles.
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
- `Marquee` and a rolling-digit counter (require a public rectangular clip modifier; only CornerRadius clips descendants today).
- `Magnetic` pointer-following content (requires a composition-time node size hook or normalized hover positions; HoverEvent reports node-local coordinates only).
- Shake and attention gestures driven by `MotionController` with velocity handoff.
- Held-Space scale feedback for keyboard activation (requires `InteractionState` exposure to composition).
- Shared `InteractionCallbacks` type for widgets that need to mirror state into application models.
