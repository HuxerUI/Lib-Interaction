# Interaction preview

This demo follows the other HuxerUI Lib projects: `examples/preview`, an `example_interaction` target created by `huxerui_add_app`, and `huxerui_use_library` linked to the library's public target. Shared application content lives in `src/app.cpp`; `src/application.cpp` declares the application. Platform entry files under `platform/` call `RunApplication()`. `HuxerUIProject.cmake` describes the independent app plan and initializes its compiler.

Features: a spring-driven pressable card with a click counter, a hover-lift tile, a pulsing status tile with opacity motion, a ripple card that observes presses without consuming clicks, and a light/dark theme toggle. All motion respects the framework's reduced-motion policy.

Build from the repository root:

```sh
cmake -S . -B build -DHUXERUI_HOME=/path/to/HuxerUI -DHUXERUI_INTERACTION_BUILD_EXAMPLES=ON
cmake --build build --config Release --target example_interaction --parallel
```

Or build the demo as an independent application:

```sh
cmake -S examples/preview -B build-preview -DHUXERUI_HOME=/path/to/HuxerUI
cmake --build build-preview --config Release --parallel
```

On Visual Studio generators the root build produces `build/examples/preview/Release/example_interaction.exe`. The independent build produces `build-preview/Release/example_interaction.exe`. Launch from your desktop or terminal. The host build and interaction tests are verified on Windows; other platform entry files require their SDK toolchains and remain unverified here.
