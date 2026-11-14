# Repository structure

Lib-Interaction follows the same rule as Flutter, GTK, and Qt: one concept per file, dependencies point
one way, and tests and samples live in their own layers. The public include path, implementation directory,
and test location should make the owning family obvious without opening the file.

## Layout

```text
include/huxerui/
  interaction.h                       # canonical umbrella; keep this stable
  interaction/
    press/                            # Pressable, LongPressable, MultiClickable, SecondaryClickable, DoubleClickable, Ripple
    hover/                            # HoverLift, HoverHighlight, Spotlight, HoverCard
    motion/                           # Pulsing, Popped, Shaken, Shimmer, Fireworks, RollingNumber, RevealOnTap
    gesture/                          # DragSpring, InertialDrag, PanZoom, PinchZoom, SwipeActions,
                                      # Dismissible, PressDragHandle, ResizeHandle
    reorder/                          # Reorderable, ReorderableGrid
    dragdrop/                         # Draggable, DropZone, FileDropZone
    scroll/                           # ScrollProgress, ScrollEdgeFade
    overlay/                          # ContextMenuArea
    accessibility/                    # SemanticActions, KeyboardShortcut
src/                                  # mirrors the include families; header-only widgets have no .cpp
examples/preview/src/
  app.cpp                             # gallery scaffold: theme, header, section order only
  demos/demos.h                       # DemoState + DemoContext + section declarations
  demos/<family>.cpp                  # one demo family per file
tests/
  support/test.h                      # CHECK, Throws, Run, Center, Near
  widget/                             # validation and composed-widget UI contracts
  advanced/                           # cross-widget gesture and state flows
  preview/                            # gallery smoke test
  api/api_surface_test.cpp            # public umbrella compile/link smoke
docs/
  design.md                           # widget catalog and motion conventions
  structure.md                        # this file
  testing.md                          # test layers and conventions
  open-questions.md                   # reviewed limitations and decisions
```

## Dependency rules

- `include/huxerui/interaction/*` may include HuxerUI core headers, but never this repository's
  `examples/` or `tests/`.
- Composed widgets may use retained modifiers and framework primitives. Retained paint modifiers stay
  paint-only: they do not consume pointer sequences and do not change layout or hit testing.
- No widget talks to a platform API directly. Platform entry points live in `examples/preview/platform/`.
- Public declarations belong in `include/`; implementation helpers stay in anonymous namespaces in the
  matching `src/` file. Do not add a public header for a helper that is not part of the contract.
- `include/huxerui/interaction.h` is the canonical entry point. Grouped headers are also supported, so a
  header move is an API change: update the umbrella, every implementation include, and the documentation.

## Adding a widget

1. Add the header under the closest `include/huxerui/interaction/<family>/`.
2. Add the implementation under the mirrored `src/<family>/`.
3. Export it from `include/huxerui/interaction.h`.
4. Add the Doxygen contract, validation rules, and `operator==` for any style type.
5. Add the entry to the widget catalog in `docs/design.md`.
6. Add validation and behavior coverage under the matching `tests/widget/` family.
7. Add the source to the explicit list in `CMakeLists.txt`.
8. If the widget deserves gallery space, add a section under `examples/preview/src/demos/` and register it
   in `app.cpp` and the preview test target.

## Adding a demos section

A demo is not part of the library. Keep its private vector assets, layout constants, and state helpers in
`examples/preview/src/demos/<family>.cpp`. `DemoState` holds only state that must be shared across sections;
`DemoContext` carries the resolved theme and viewport flags. `app.cpp` should remain a section list and
theme scaffold.

## Build boundary

There is one library target, `huxerui_interaction`. Grouping directories does not create separate CMake
targets: at this size a single target keeps linking and export simple. Split targets only when consumers
need a smaller dependency surface or build time demands it; prefer `interaction_core` +
`interaction_widgets` over one target per family.
