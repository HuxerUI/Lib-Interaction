# Open questions

Decisions made while scaffolding Lib-Interaction that deserve review. Each item records what was chosen, why, and what should be revisited. Nothing here blocks using the library; the choices are conservative and reversible.

## Repo and project identity

1. **Repository location: `HuxerUI/Lib-Interaction`.** Four of five sibling Lib repositories live in the `HuxerUI` organization (Lib-CodeEditor is personal under `FarnaHerry`). This repo was created in the org to match the majority pattern. Confirm the org placement is intended for this library.

2. **Library target and namespace: `huxerui_interaction` / `HuxerUI::Interaction` / `huxerui::interaction`.** Follows the `HuxerUI::QRCode` + `huxerui::qrcode` pattern. If "Interaction" is expected to grow into a broader grab bag, consider splitting into `HuxerUI::Motion` (pure animation helpers) and `HuxerUI::Interaction` (input-driven) before the API surface grows.

3. **CI pins HuxerUI revision `56eb5ab225997cffc83ce8f2736d1fd52c530b70`** — the current `HuxerUI/HuxerUI` main head on GitHub at scaffold time. The first CI attempt pinned the local checkout's HEAD (`42dc57f`), which failed with `upload-pack: not our ref`: the local HuxerUI clone tracks the `FarnaHerry/HuxerUI` fork and contains unpushed merge commits, so only upstream-reachable revisions work for CI. The local build was repeated against the pinned upstream revision before publishing. Decide whether all Lib repos should track one shared verified revision, and whether the fork's local commits should be pushed to the upstream org repo.

4. **No standalone core mode.** Lib-QRCode can build without HuxerUI because it has a pure codec; every Lib-Interaction widget is view-bound, so the CMake project has no `BUILD_VIEW=OFF` path and CI has a single native job. Confirm this is acceptable, or name a HuxerUI-free piece (e.g., spring math) worth extracting.

## Widget API decisions

5. **Press scale semantics.** Default `PressableStyle` is rest `1.0`, hover `1.03`, press `0.97` with a 0.16 s ease-out tween. These numbers were chosen from common UI practice, not from an existing HuxerUI component; the SDK has no canonical press-scale constant yet. If HuxerUI later adds official press tokens, they should replace these defaults.

6. **Press tracking relies on `ViewEvents::Pointer` phases.** If the framework delivers `Up` to a node after the pointer leaves its bounds, this is sufficient; if not, a fast press-out could leave the widget stuck in the pressed scale until the next cancel. The retained-extension path (`NodeExtension::OnPointer` with `Capture`) would be robust but changes event consumption semantics. Needs a manual verification pass with real pointer hardware (touch drag-out, mouse press-out, multi-button).

7. **Disabled-state recovery.** When a node becomes disabled mid-point-press, events stop arriving; the widget still relies on the framework delivering a cancel phase so `pressed_scale` clears. Held-Space scale now clears on focus loss (`FocusChanged`), but a retained `OnInteraction` extension could make pointer recovery independent of the cancel path. Options: accept the cancel dependency, or add that `Enabled`-aware probe.

8. **`HitTest` on the Ripple extension returns true inside node bounds.** This makes any rippled node an input target, which is required to observe presses on otherwise inert views. Verify this does not surprise layouts that rely on inert nodes being click-transparent (e.g., an overlay decoration over an interactive sibling). An opt-out flag in `RippleStyle` (e.g., `observe_only = false` meaning no HitTest override) is a possible follow-up.

9. **Ripple paint uses `PaintAboveContent` with `PushClip(bounds, RippleStyle::corner_radii)`.** If a node has large padding, the ripple covers the padding area too (bounds, not content bounds). Confirm the intended surface: bounds (current) vs `ContentBounds()`. The corner radii are style-controlled because the pinned upstream revision exposes no node corner-radii accessor; the local HuxerUI fork has `ViewNode::ResolvedCornerRadii()`, so once that lands upstream the default could resolve the host surface's rounding automatically.

10. **`Pulsing` uses unbounded reverse playback of declarative targets.** This depends on the documented behavior that infinite reverse iterations alternate between the start and target values indefinitely. Worth a visual check that the oscillation does not stall after the first cycle on any backend, and that reduced motion pins it to the target (not the start) value.

11. **No `Expandable` widget yet.** True collapse requires animating layout size, which `Transition` explicitly does not do. Candidate SDK features that would unblock it: animated content size, animated clip height, or a layout-affecting extension. Deferred until the SDK exposes one; recorded here so it is not forgotten.

## Preview and tests

12. **Preview app is a single scrolling gallery** (twelve keyed demo tiles, theme toggle, and click/hold/double-click counters). It exercises the original widget gallery and feeds the preview test; the 2026-11 swipe, drag/drop, transform, scroll, and rolling-number widgets are covered by the advanced test target instead. If the preview set grows, consider a `Select`-based gallery like the other Lib previews.

13. **Interaction tests cover dispatch, layout, and gesture behavior, but not painted pixels.** Click routing, PinchZoom wheel clamping, Reorderable indices, long-press recognition, double-tap recognition, swipe-action reveal/activation, context-menu request delivery, typed drag/drop delivery, file-drop-zone construction, PanZoom pan/scale/rotation, InertialDrag velocity projection, scroll-container motion, RollingNumber digit movement, semantic actions, keyboard shortcuts, triple-tap recognition, secondary-click routing, press-drag recognition, dismiss exit completion, horizontal reorder, roving focus traversal, reveal mutation, and HoverCard mounting are asserted through `UiTest`; ripple/spotlight/shimmer painting is still verified only by the code paths executing without failure. Screenshot-level assertions need the UI testing library's snapshot or pixel facilities; check what `UiSnapshot` comparison supports before investing.

## Manual verification follow-ups

- Run the preview app (`example_interaction`) manually and eyeball the preview widget set, light and dark.
- Confirm `PointerResult::Observe` ripples on a `Button` do not interfere with the button's own press indication.
- Review item 2 (naming) and item 3 (shared pinned revision) with the team.

## Batch two findings

14. **Entrance animations (FadeIn/slide-in on mount) are not implementable with public API yet.** `TransitionExtension` resolves an `Animated` progress to its target immediately on first mount (verified in the SDK source), so a `Transition{AnimateTo(1.0F)}.Opacity(0, 1)` appears fully settled. A proper appear transition needs either an SDK mount hook, an `Animated` "from" value, or an appearance policy. Recorded as a framework request; no Lib workaround is planned.

15. **`DragSpring` does not hand terminal drag velocity to the return spring.** The declarative `Animated` target form has no velocity input (`MotionController::Seek` does, but that is retained-only). `InertialDrag` now uses terminal velocity to project a target, but it is not velocity-continuous. Follow-up options: a retained executor widget, or an SDK velocity parameter on `AnimateTo`.

16. **`Spotlight` invalidates the behind-content paint sequence on every hover move.** Repainting one gradient rect per pointer move is expected to be cheap, but confirm on the Windows backend that hover-move repaints do not visibly cost. If they do, throttle by distance or move the glow to the framework's per-frame paint path.

17. **`UseShake` is defined inline in the header on purpose.** The composable-source scan processes library `.cpp` files; a plain (unmarked) function calling composition hooks from a scanned source risks transformer rejection, so the hook follows the SDK's inline-header precedent (`UseState` and friends live in `state.h`). Confirm the team is comfortable with this boundary: hooks in headers, widgets and extensions in sources.

18. **`ShakeTrigger::Trigger` is legal only from handlers/tasks** because `TaskScope::Launch` throws during composition (verified in `task.h`). The header documents it; consider whether a composition-safe `RequestTrigger()` variant is worth adding later.

