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

- **Foundation Upgrade Batch 1 (`Expandable`, `Appear`, `Magnetic`, `NestedScrollConnection`, predicate `DropZone<T>`)**:
  - `Expandable` implements true layout-size collapse/expand using custom `Layout<ExpandableLayout>` + `MotionController` +
    `ClipChildren()`, omitting `Place()` when settled closed so collapsed children remain mounted without participating
    in input, focus, or semantics.
  - `Appear` triggers mount entrance from `0.0F` to `1.0F` via a 0-dependency `Lifecycle` hook paired with
    `Transition`.
  - `Magnetic` splits a stationary outer hover-probe `Stack` (`ViewNode::Bounds()` + `HoverHitTest`/`OnHover`) from inner
    `Offset`/`Scale` transforms to avoid hover feedback jitter.
  - `NestedScrollConnection` wraps `NodeExtension::OnPreScroll`/`OnPostScroll`/`OnPreFling`/`OnPostFling` with signed
    clamping, paired with `ScrollHideController`.
  - `DropZone<T>` exposes predicate-filtered payload matching via `DropTarget::Accepts<T>(predicate)`.
- **Foundation Upgrade Batch 2 (`ShortcutMap`, `RovingFocus`, `Popover`, `Copyable`, `SharedMorph`)**:
  - `ShortcutMap` consolidates multiple `KeyChord` entries into a single `ViewEvents::KeyIntercept` listener.
  - `RovingFocusGroup` / `RovingFocusItem` / `RovingFocusController` pair `Environment` registration with
    `FocusRequester` + `NodeExtension::OnFocusChanged` (preserving child focus handlers) and `Lifecycle` token cleanup.
  - `Popover` / `PopoverController` deliver click-triggered popovers synchronized with `PopupContext::Dismiss()` and
    outside/Escape dismissal via guarded `TaskScope::Post` cleanup.
  - `Copyable` integrates `UseApplication().Clipboard()` with `TaskScope::Delay` timed feedback reset.
  - `UseSharedMorph` / `MorphScope` / `MorphElement` / `MorphBounds` expose HuxerUI's local shared-element transition
    scope and pipelined markers with one-shot timing validation.
- **Back Interception (`BackHandler`, `PredictiveBack`)**:
  - `BackHandler` and `PredictiveBack` wrap `NodeExtension::OnBack(ViewNode&, const BackEvent&)` so dynamic
    `enabled = false` bypasses Back dispatch to enclosing scopes or platform navigation without unmounting content,
    while capturing predictive `Begin` → `Update` → `Cancel`/`Commit` transactions cleanly.
- **Scroll Section Tracking (`ScrollSpyArea`, `ScrollSpySection`, `ScrollSpyController`)**:
  - Pairs `Environment` section registration with `PrepareGeometry` (`NodeExtension::PrepareGeometry` reading
    `PresentationBounds()`) and coalesced `TaskScope::Post` synchronization (never posting during `Update()` /
    composition or cleanup), supporting vertical/horizontal tracking, bottom-of-scroll last-section fallback, and
    configurable `ScrollJumpBehavior::Smooth` (driven via retained `MotionController` + `scroll_motion`, auto-canceled
    on user scroll interruption) or `ScrollJumpBehavior::Instant` `ScrollToSection(index)` jump alignment.
- **Anchor-Snapping Floating Drag (`SnapDrag`, `SnapDragController`)**:
  - Combines a non-blocking `SnapDragStageLayout` with `MotionController::Seek(pos, vel)` + `AnimateTo()` to provide
    velocity-continuous corner (`Corners`), edge (`HorizontalEdges` / `VerticalEdges`), and normalized custom
    (`Custom`) dock snapping while preserving click-through outside the floating item.
- **Stacked Card Deck (`SwipeDeck`, `SwipeDeckController`)**:
  - Coordinates top-card drag/tilt/exit and background depth promotion via `MotionController::Seek` and stable keyed
    stack reconciliation (`Key(card_index)`), allowing smooth `Swipe()`, `Rewind()` history undo, and `Reset()` even
    when transitioning to or from `empty_builder`.
- **Accelerating Press-and-Hold Repeater (`HoldRepeat`, `HoldRepeatController`)**:
  - Drives initial-delay and geometrically accelerating repeat ticks through `TaskScope::Launch` + `TaskScope::Delay`,
    combining primary-pointer capture (`cancel_on_leave` bounds monitoring), keyboard Space/Enter repeat swallowing,
    live composition closure refresh, dynamic `Enabled(false)` cancellation, and unmount teardown cleanup.
- **Sticky Section Pinning (`StickyHeaderArea`, `StickyHeaderSection`, `StickyHeaderList`, `StickyHeaderController`)**:
  - Pairs `StickyHeaderStageLayout` (`ClipChildren()`, unplaced overlay when unpinned) with `Environment` section
    registration, `PrepareGeometry` scroll-invariant content bounds, `ScrollController::Metrics()` subscription on the
    overlay scope only (keeping list body subtrees out of scroll recomposition), push-up collision offset calculations,
    and shared `internal::ScrollAnimator` `Smooth`/`Instant` jumps.
- **Scroll Parallax (`ScrollParallaxArea`, `ScrollParallax`)**:
  - Uses stationary `ScrollParallaxPassThroughLayout` wrappers to measure viewport/item `PresentationBounds()` cleanly
    without transform feedback loops, computing normalized `progress`/`visibility`, peak center scale, edge opacity fade,
    standalone `ScrollController` hero shifts, and `FrameInfo::reduced_motion` suppression.

## Remaining framework-bound questions

1. **Shared pinned HuxerUI revision in CI**: CI currently checks out `56eb5ab225997cffc83ce8f2736d1fd52c530b70`.
   Decide whether sibling `Lib-*` repositories should track one shared verified revision.
2. **Press tracking on fast drag-out without cancel**: Relies on HuxerUI delivering `Up` or `Cancel` after pointer exit;
   retained `PointerResult::Capture` would alter event consumption semantics.
3. **Host corner-radii auto-resolution on retained paint modifiers (`Ripple`, `Spotlight`, `HoverHighlight`,
   `Shimmer`)**: Corner radii remain style-configured until `ViewNode::ResolvedCornerRadii()` lands in upstream
   `HuxerUI/HuxerUI`.
4. **SDK primitives needed for remaining roadmap candidates**:
   - Velocity-continuous `DragSpring` / `InertialDrag` handoff needs an initial-velocity parameter on `AnimateTo` or a
     retained transform extension hook.
