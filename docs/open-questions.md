# Open questions & resolved decisions

Architectural decisions reviewed during scaffolding and the v0.2 separation refactor.

## Resolved in v0.2 refactor

- **Interaction vs. pure motion boundary (formerly #2)**: Non-interactive visual and value-driven components (`Pulsing`,
  `Shimmer`, `Fireworks`, `RollingNumber`, `UsePop`/`Popped`, `UseShake`/`Shaken`, `UseRevealOnTap`) are now isolated
  under `include/huxerui/interaction/visual/` and `src/visual/`, keeping input-driven families distinct while retaining
  the single `HuxerUI::Interaction` CMake target.
- **Headless state & controlled handles (formerly roadmap item)**: Added `InteractiveSurface`, `Hoverable`, builder
  overloads on `DropZone`/`FileDropZone`/`Dismissible`, plus `SwipeController`, `PanZoomController`,
  `HoverCardController`, and environment-bound `ReorderHandle`.
- **Retained modifier equality & HoverCard dismissal sync**: Added `operator==` to all public retained modifiers and
  replaced raw `PopupOptions` in `HoverCardStyle` with value-comparable fields while wiring internal
  `on_dismiss_request` synchronization.
- **Trigger header hygiene (formerly #17)**: `PopTrigger` and `ShakeTrigger` now hide `State<T>` and `TaskScope` behind
  `std::shared_ptr` implementation structs in `src/visual/`, removing `<huxerui/state.h>` and `<huxerui/task.h>` from
  public headers and making default-constructed handle equality `noexcept` and runtime-free.
- **Primary-button press filtering & non-clobbering hit-test**: `InteractiveSurface` / `Pressable` filter pointer press
  tracking to `PointerButton::Primary`, and `ContextMenuArea` uses an internal `HitTest`-only `NodeExtension` instead
  of overwriting `ViewEvents::Pointer`.

## Remaining framework-bound questions

1. **Shared pinned HuxerUI revision in CI**: CI currently checks out `56eb5ab225997cffc83ce8f2736d1fd52c530b70`.
   Decide whether sibling `Lib-*` repositories should track one shared verified revision.
2. **Press tracking on fast drag-out without cancel**: Relies on HuxerUI delivering `Up` or `Cancel` after pointer exit;
   retained `PointerResult::Capture` would alter event consumption semantics.
3. **Host corner-radii auto-resolution on retained paint modifiers (`Ripple`, `Spotlight`, `HoverHighlight`,
   `Shimmer`)**: Corner radii remain style-configured until `ViewNode::ResolvedCornerRadii()` lands in upstream
   `HuxerUI/HuxerUI`.
4. **SDK primitives needed for roadmap candidates**:
   - `Expandable` / `Collapse` needs animated layout size or animated clip height in the SDK.
   - `Appear` / entrance transitions need a mount appearance policy in `TransitionExtension`.
   - Velocity-continuous `DragSpring` / `InertialDrag` handoff needs an initial-velocity parameter on `AnimateTo` or a
     retained transform extension hook.
