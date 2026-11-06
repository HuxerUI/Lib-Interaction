# Open questions

Decisions made while scaffolding Lib-Interaction that deserve review. Each item records what was chosen, why, and what should be revisited. Nothing here blocks using the library; the choices are conservative and reversible.

## Repo and project identity

1. **Repository location: `HuxerUI/Lib-Interaction`.** Four of five sibling Lib repositories live in the `HuxerUI` organization (Lib-CodeEditor is personal under `FarnaHerry`). This repo was created in the org to match the majority pattern. Confirm the org placement is intended for this library.

2. **Library target and namespace: `huxerui_interaction` / `HuxerUI::Interaction` / `huxerui::interaction`.** Follows the `HuxerUI::QRCode` + `huxerui::qrcode` pattern. If "Interaction" is expected to grow into a broader grab bag, consider splitting into `HuxerUI::Motion` (pure animation helpers) and `HuxerUI::Interaction` (input-driven) before the API surface grows.

3. **CI pins HuxerUI revision `42dc57f10d2713f40272be393bafb6200005ca83`** — the local HuxerUI checkout used to verify this scaffold (yesterday's merge of the text field caret fix). Lib-QRCode pins an older revision (`cadb1696`). Decide whether all Lib repos should track one shared verified revision.

4. **No standalone core mode.** Lib-QRCode can build without HuxerUI because it has a pure codec; every Lib-Interaction widget is view-bound, so the CMake project has no `BUILD_VIEW=OFF` path and CI has a single native job. Confirm this is acceptable, or name a HuxerUI-free piece (e.g., spring math) worth extracting.

## Widget API decisions

5. **Press scale semantics.** Default `PressableStyle` is rest `1.0`, hover `1.03`, press `0.97` with a 0.16 s ease-out tween. These numbers were chosen from common UI practice, not from an existing HuxerUI component; the SDK has no canonical press-scale constant yet. If HuxerUI later adds official press tokens, they should replace these defaults.

6. **Press tracking relies on `ViewEvents::Pointer` phases.** If the framework delivers `Up` to a node after the pointer leaves its bounds, this is sufficient; if not, a fast press-out could leave the widget stuck in the pressed scale until the next cancel. The retained-extension path (`NodeExtension::OnPointer` with `Capture`) would be robust but changes event consumption semantics. Needs a manual verification pass with real pointer hardware (touch drag-out, mouse press-out, multi-button).

7. **Disabled-state recovery.** When a node becomes disabled mid-press, events stop arriving; the widget may hold the pressed scale until re-enabled. Options: accept and document (current choice), or add an `Enabled`-aware retained extension that resets presentation state on `OnInteraction`. The latter requires observing `InteractionState` through a NodeExtension — doable with the same descriptor machinery as Ripple.

8. **`HitTest` on the Ripple extension returns true inside node bounds.** This makes any rippled node an input target, which is required to observe presses on otherwise inert views. Verify this does not surprise layouts that rely on inert nodes being click-transparent (e.g., an overlay decoration over an interactive sibling). An opt-out flag in `RippleStyle` (e.g., `observe_only = false` meaning no HitTest override) is a possible follow-up.

9. **Ripple paint uses `PaintAboveContent` with `PushClip(bounds, ResolvedCornerRadii())`.** If a node has large padding, the ripple covers the padding area too (bounds, not content bounds). Confirm the intended surface: bounds (current) vs `ContentBounds()`.

10. **`Pulsing` uses unbounded reverse playback of declarative targets.** This depends on the documented behavior that infinite reverse iterations alternate between the start and target values indefinitely. Worth a visual check that the oscillation does not stall after the first cycle on any backend, and that reduced motion pins it to the target (not the start) value.

11. **No `Expandable` widget yet.** True collapse requires animating layout size, which `Transition` explicitly does not do. Candidate SDK features that would unblock it: animated content size, animated clip height, or a layout-affecting extension. Deferred until the SDK exposes one; recorded here so it is not forgotten.

## Preview and tests

12. **Preview app is intentionally minimal** (four demo tiles, theme toggle, click counter). It exercises every widget and feeds the preview test. If the widget set grows, consider a `Select`-based gallery like the other Lib previews.

13. **Interaction tests only cover click dispatch and layout sizes.** Hover and ripple visuals are verified only by the code paths executing without failure inside `UiTest`. Screenshot-level assertions (ripple visible mid-animation, scale changes during hover) need the UI testing library's snapshot or pixel facilities; check what `UiSnapshot` comparison supports before investing.

## Follow-ups for tomorrow

- Run the preview app (`example_interaction`) manually and eyeball all four widgets, light and dark.
- Confirm `PointerResult::Observe` ripples on a `Button` do not interfere with the button's own press indication.
- Review item 2 (naming) and item 3 (shared pinned revision) with the team.
