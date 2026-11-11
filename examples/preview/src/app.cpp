#include "app.h"
#include <huxerui/interaction.h>

#include <huxerui/environment.h>
#include <huxerui/vector.h>

#include <algorithm>
#include <string>
#include <utility>

namespace interaction_demo {
using namespace huxerui;
using namespace huxerui::interaction;

namespace {

/// The three theme modes offered by the preview header buttons.
enum class ThemeMode { System, Light, Dark };

const StrokeStyle kIconStroke{.width = 1.6F, .cap = StrokeCap::Round, .join = StrokeJoin::Round};

/// Circle outline with the filled right half, marking the follow-system theme mode.
const VectorAsset& ThemeSystemIcon() {
  static const VectorAsset icon = VectorAsset::Create({24.0F, 24.0F}, [](VectorBuilder& builder) {
    const Color ink = Color::Black();
    const float radius = 8.2F;
    Path outline;
    outline.MoveTo({12.0F, 12.0F - radius});
    outline.ArcTo({radius, radius}, 0.0F, ArcSize::Small, ArcDirection::Clockwise, {12.0F, 12.0F + radius});
    outline.ArcTo({radius, radius}, 0.0F, ArcSize::Small, ArcDirection::Clockwise, {12.0F, 12.0F - radius});
    outline.Close();
    builder.StrokePath(outline, ink, kIconStroke);
    Path half;
    half.MoveTo({12.0F, 12.0F - radius});
    half.ArcTo({radius, radius}, 0.0F, ArcSize::Small, ArcDirection::Clockwise, {12.0F, 12.0F + radius});
    half.Close();
    builder.FillPath(half, ink);
  });
  return icon;
}

/// Sun with eight round-capped rays, marking the light theme mode.
const VectorAsset& ThemeLightIcon() {
  static const VectorAsset icon = VectorAsset::Create({24.0F, 24.0F}, [](VectorBuilder& builder) {
    const Color ink = Color::Black();
    Path disc;
    disc.MoveTo({12.0F, 8.2F});
    disc.ArcTo({3.8F, 3.8F}, 0.0F, ArcSize::Small, ArcDirection::Clockwise, {12.0F, 15.8F});
    disc.ArcTo({3.8F, 3.8F}, 0.0F, ArcSize::Small, ArcDirection::Clockwise, {12.0F, 8.2F});
    disc.Close();
    builder.StrokePath(disc, ink, kIconStroke);
    const float inner = 6.3F;
    const float outer = 9.3F;
    const float diagonal_in = inner * 0.7071F;
    const float diagonal_out = outer * 0.7071F;
    Path rays;
    rays.MoveTo({12.0F + inner, 12.0F}).LineTo({12.0F + outer, 12.0F});
    rays.MoveTo({12.0F - inner, 12.0F}).LineTo({12.0F - outer, 12.0F});
    rays.MoveTo({12.0F, 12.0F - inner}).LineTo({12.0F, 12.0F - outer});
    rays.MoveTo({12.0F, 12.0F + inner}).LineTo({12.0F, 12.0F + outer});
    rays.MoveTo({12.0F + diagonal_in, 12.0F - diagonal_in}).LineTo({12.0F + diagonal_out, 12.0F - diagonal_out});
    rays.MoveTo({12.0F + diagonal_in, 12.0F + diagonal_in}).LineTo({12.0F + diagonal_out, 12.0F + diagonal_out});
    rays.MoveTo({12.0F - diagonal_in, 12.0F + diagonal_in}).LineTo({12.0F - diagonal_out, 12.0F + diagonal_out});
    rays.MoveTo({12.0F - diagonal_in, 12.0F - diagonal_in}).LineTo({12.0F - diagonal_out, 12.0F - diagonal_out});
    builder.StrokePath(rays, ink, kIconStroke);
  });
  return icon;
}

/// Crescent moon opening toward the top-right, marking the dark theme mode.
const VectorAsset& ThemeDarkIcon() {
  static const VectorAsset icon = VectorAsset::Create({24.0F, 24.0F}, [](VectorBuilder& builder) {
    Path moon;
    moon.MoveTo({12.0F, 3.0F});
    moon.ArcTo({6.0F, 6.0F}, 0.0F, ArcSize::Small, ArcDirection::CounterClockwise, {21.0F, 12.0F});
    moon.ArcTo({9.0F, 9.0F}, 0.0F, ArcSize::Large, ArcDirection::Clockwise, {12.0F, 3.0F});
    moon.Close();
    builder.StrokePath(moon, Color::Black(), kIconStroke);
  });
  return icon;
}

/// Material's baseline palette is purple; override the primary family with the framework's blue roles.
ThemeSpec BlueThemeSpec(bool dark) {
  ThemeSpec spec = dark ? MaterialDarkThemeSpec() : MaterialLightThemeSpec();
  spec.colors.primary = dark ? Color::Rgb(168, 200, 250) : Color::Rgb(31, 111, 235);
  spec.colors.on_primary = dark ? Color::Rgb(11, 46, 96) : Color::White();
  spec.colors.primary_container = dark ? Color::Rgb(18, 74, 163) : Color::Rgb(221, 235, 255);
  spec.colors.on_primary_container = dark ? Color::Rgb(208, 227, 250) : Color::Rgb(12, 65, 128);
  return spec;
}

} // namespace

[[huxerui::composable]]
View PreviewApp() {
  auto clicks = UseState(0);
  auto holds = UseState(0);
  auto doubles = UseState(0);
  auto burst = UseState(FireworksBurst{});
  auto lastReorder = UseState(std::string{"drop to reorder"});
  auto gridLast = UseState(std::string{"drop a cell"});
  auto pane_width = UseState(164.0F);
  auto shake = UseShake();
  auto pop = UsePop();
  auto mode = UseState(ThemeMode::System);
  const bool dark = mode.Get() == ThemeMode::Dark
      || (mode.Get() == ThemeMode::System && UseSystemThemeBrightness() == ThemeBrightness::Dark);
  return MaterialTheme(BlueThemeSpec(dark), Scope([=] {
    const auto& theme = UseTheme();
    const ViewportClass viewport = UseViewportClass();
    const bool compact = viewport == ViewportClass::Compact;
    const bool expanded = viewport == ViewportClass::Expanded;
    const auto tile = Frame{.width = 240.0F, .height = 120.0F};
    const auto card_label = Foreground(theme.colors.on_primary_container);
    const auto reorder_tile = [&theme](const char* label) {
      return Stack { Text(label, TextRole::Label)
                         .With(Foreground(theme.colors.on_primary_container)) }
          .With(Frame{.width = 240.0F, .height = 48.0F},
                Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                Background{theme.colors.primary_container}, CornerRadius(12.0F));
    };
    const auto grid_tile = [&theme](const char* label) {
      return Stack { Text(label, TextRole::Label)
                         .With(Foreground(theme.colors.on_primary_container)) }
          .With(Frame{.width = 96.0F, .height = 64.0F},
                Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                Background{theme.colors.primary_container}, CornerRadius(12.0F));
    };
    const auto theme_button = [&theme, mode](VectorAsset icon, const char* label, ThemeMode value) -> View {
      View button = IconButton(std::move(icon), label).OnClick([mode, value] { mode = value; });
      if(mode.Get() == value) {
        button = std::move(button).With(Background{theme.colors.primary}, CornerRadius(20.0F));
      }
      return button;
    };
    return ScrollView(Column {
      Row {
        Column {
          Text("Interaction", TextRole::Title),
          Text("Interactive animation widgets for HuxerUI: presses, hovers, pulses, ripples, drags, and shakes.",
               TextRole::Label),
        }.With(Grow{}, Spacing(6.0F)),
        theme_button(ThemeSystemIcon(), "Follow system theme", ThemeMode::System).Key("theme-system"),
        theme_button(ThemeLightIcon(), "Light theme", ThemeMode::Light).Key("theme-light"),
        theme_button(ThemeDarkIcon(), "Dark theme", ThemeMode::Dark).Key("theme-dark"),
      }.With(Spacing(4.0F), CrossAlign(CrossAxisAlignment::Center)),
      Text("Press and hover", TextRole::Label).With(Foreground(theme.colors.on_surface_variant)),
      Flow {
        Pressable(
            Stack {
              Column {
                Text("Press me", TextRole::Label).With(card_label),
                Text("Spring scale on hover and press", TextRole::Label).With(card_label),
              }.With(Spacing(4.0F), CrossAlign(CrossAxisAlignment::Center)),
            }.With(tile, Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                   Background{theme.colors.primary_container}, CornerRadius(14.0F)),
            {.animation = SpringSpec{.stiffness = 320.0F, .damping_ratio = 0.7F}},
            [=] { clicks += 1; })
            .Key("pressable-demo"),
        HoverLift(
            Stack {
              Column {
                Text("Hover me", TextRole::Label).With(card_label),
                Text("Lift on hover", TextRole::Label).With(card_label),
              }.With(Spacing(4.0F), CrossAlign(CrossAxisAlignment::Center)),
            }.With(tile, Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                   Background{theme.colors.primary_container}, CornerRadius(14.0F)),
            {.lift = {0.0F, -6.0F}, .animation = TweenSpec{.duration = 0.2, .easing = Easing::EaseOut}})
            .Key("lift-demo"),
        Shaken(
            Stack { Text("Shake me", TextRole::Label).With(card_label) }.With(
                Frame{.width = 240.0F, .height = 72.0F},
                Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                Background{theme.colors.primary_container}, CornerRadius(14.0F)),
            shake)
            .OnClick([shake] { shake.Trigger(); })
            .With(PointerCursor{PointerCursorKind::Hand})
            .Key("shake-demo"),
      }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Center)),
      Text("Paint and pulse", TextRole::Label).With(Foreground(theme.colors.on_surface_variant)),
      Flow {
        Pulsing(
            Stack { Text("Live", TextRole::Label).With(card_label) }.With(
                Frame{.width = 96.0F, .height = 40.0F},
                Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                Background{theme.colors.primary_container}, CornerRadius(20.0F)),
            {.scale_amplitude = 0.05F, .duration = 0.9})
            .Key("pulse-demo"),
        Stack { Text("Ripple card", TextRole::Label).With(card_label) }.With(
                Frame{.width = 240.0F, .height = 96.0F},
                Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                Background{theme.colors.primary_container},
                Ripple{RippleStyle{.color = Color::Rgb(255, 255, 255, 0.22F), .corner_radii = 14.0F}},
                CornerRadius(14.0F))
            .Key("ripple-demo"),
        Stack { Text("Spotlight", TextRole::Label).With(card_label) }.With(
                Frame{.width = 240.0F, .height = 96.0F},
                Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                Background{theme.colors.primary_container}, CornerRadius(14.0F),
                Spotlight{SpotlightStyle{.corner_radii = 14.0F}})
            .Key("spotlight-demo"),
        Stack { Text("Hover row", TextRole::Label).With(card_label) }.With(
                Frame{.width = 240.0F, .height = 96.0F},
                Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                Background{theme.colors.primary_container}, CornerRadius(14.0F),
                HoverHighlight{HoverHighlightStyle{.corner_radii = 14.0F}}),
      }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Center)),
      Text("Gesture", TextRole::Label).With(Foreground(theme.colors.on_surface_variant)),
      Flow {
        LongPressable(
            Stack { Text("Hold me", TextRole::Label).With(card_label) }.With(
                Frame{.width = 240.0F, .height = 96.0F},
                Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                Background{theme.colors.primary_container}, CornerRadius(14.0F)),
            {.show_progress = true},
            [=] { holds += 1; })
            .Key("longpress-demo"),
        DoubleClickable(
            Stack { Text("Double-click me", TextRole::Label).With(card_label) }.With(
                Frame{.width = 240.0F, .height = 96.0F},
                Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                Background{theme.colors.primary_container}, CornerRadius(14.0F)),
            {},
            [=](const MultiTapEvent& event) {
              doubles += 1;
              burst = FireworksBurst{.id = burst.Get().id + 1, .local_position = event.position};
            })
            .With(Fireworks{FireworksStyle{.corner_radii = 14.0F}, burst.Get()})
            .Key("doubleclick-demo"),
        Popped(
            Stack { Text("Pop", TextRole::Label).With(Foreground(theme.colors.on_primary)) }.With(
                Frame{.width = 96.0F, .height = 40.0F},
                Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                Background{theme.colors.primary}, CornerRadius(20.0F)),
            pop)
            .OnClick([pop] { pop.Trigger(); })
            .With(PointerCursor{PointerCursorKind::Hand})
            .Key("pop-demo"),
        Stack { Text("Loading…", TextRole::Label).With(card_label) }.With(
                Frame{.width = 240.0F, .height = 96.0F},
                Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                Background{theme.colors.primary_container}, CornerRadius(14.0F),
                Shimmer{ShimmerStyle{.corner_radii = 14.0F}})
            .Key("shimmer-demo"),
      }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Center)),
      Text("Reorder", TextRole::Label).With(Foreground(theme.colors.on_surface_variant)),
      Flow {
        Reorderable(
            {reorder_tile("1 · Drag me"), reorder_tile("2 · over a slot"), reorder_tile("3 · and drop")},
            {},
            [=](std::size_t from, std::size_t to) {
              lastReorder = "moved slot " + std::to_string(from) + " → " + std::to_string(to);
            })
            .With(Frame{.width = 240.0F})
            .Key("reorder-demo"),
        Text("last: " + lastReorder.Get(), TextRole::Label)
            .With(Foreground(theme.colors.on_surface_variant)),
      }.With(Spacing(24.0F), CrossAlign(CrossAxisAlignment::Start)),
      Text("Layout", TextRole::Label).With(Foreground(theme.colors.on_surface_variant)),
      Flow {
        Column {
          ReorderableGrid(
              {grid_tile("A"), grid_tile("B"), grid_tile("C"), grid_tile("D"), grid_tile("E"), grid_tile("F")},
              {.cell_width = 96.0F, .cell_height = 64.0F, .spacing = 12.0F},
              [=](std::size_t from, std::size_t to) {
                gridLast = "grid " + std::to_string(from) + " → " + std::to_string(to);
              })
              .With(Frame{.width = 320.0F})
              .Key("grid-demo"),
          Text("last: " + gridLast.Get(), TextRole::Label)
              .With(Foreground(theme.colors.on_surface_variant))
              .Key("grid-last"),
        }.With(Spacing(8.0F)),
        Column {
          Row {
            Stack { Text("Sidebar", TextRole::Label).With(card_label) }.With(
                Frame{.width = pane_width.Get(), .height = 140.0F},
                Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                Background{theme.colors.surface_container_low}, CornerRadius(12.0F)),
            ResizeHandle(Axis::Horizontal, [pane_width](float delta) {
              pane_width = std::clamp(pane_width.Get() + delta, 96.0F, 280.0F);
            }).Key("resize-demo"),
            Stack { Text("Fill", TextRole::Label).With(card_label) }.With(
                Grow{}, Frame{.height = 140.0F},
                Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                Background{theme.colors.surface_container_low}, CornerRadius(12.0F)),
          }.With(CrossAlign(CrossAxisAlignment::Stretch), Frame{.width = 360.0F, .height = 140.0F}),
          Text("pane: " + std::to_string(static_cast<int>(pane_width.Get())) + " px", TextRole::Label)
              .With(Foreground(theme.colors.on_surface_variant))
              .Key("resize-width"),
        }.With(Spacing(8.0F)),
      }.With(Spacing(24.0F), CrossAlign(CrossAxisAlignment::Start)),
      Text("Drag", TextRole::Label).With(Foreground(theme.colors.on_surface_variant)),
      DragSpring(
          Stack { Text("Drag me", TextRole::Label).With(Foreground(theme.colors.on_primary)) }.With(
              Frame{.width = 140.0F, .height = 48.0F},
              Align(HorizontalAlignment::Center, VerticalAlignment::Center),
              Background{theme.colors.primary}, CornerRadius(12.0F)))
          .With(Frame{.width = expanded ? 560.0F : (compact ? 240.0F : 400.0F), .height = 72.0F},
                Background{theme.colors.surface_container_low}, CornerRadius(14.0F))
          .Key("dragspring-demo"),
      PinchZoom(
          Stack { Text("Pinch me", TextRole::Label).With(card_label) }.With(
              Frame{.width = 240.0F, .height = 72.0F},
              Align(HorizontalAlignment::Center, VerticalAlignment::Center),
              Background{theme.colors.primary_container}, CornerRadius(14.0F)))
          .Key("pinch-demo"),
      Flow {
        Text("Tap anywhere on the tiles",
             TextRole::Label).With(Foreground(theme.colors.on_surface_variant)),
        Text("Tab focuses, Enter or Space activates", TextRole::Label)
            .With(Foreground(theme.colors.on_surface_variant)),
        Text("Ctrl + wheel zooms the pinch tile", TextRole::Label)
            .With(Foreground(theme.colors.on_surface_variant)),
        Text("clicks: " + std::to_string(clicks.Get()), TextRole::Label).Key("click-count"),
        Text("holds: " + std::to_string(holds.Get()), TextRole::Label).Key("hold-count"),
        Text("double-clicks: " + std::to_string(doubles.Get()), TextRole::Label).Key("double-count"),
        Text("Motion respects the system reduced-motion preference.", TextRole::Label)
            .With(Foreground(theme.colors.on_surface_variant)),
      }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Center)),
    }.With(Padding(compact ? 16.0F : 24.0F), Spacing(20.0F), CrossAlign(CrossAxisAlignment::Stretch)))
        .With(Background{theme.colors.background}, ScrollBar());
  }));
}

} // namespace interaction_demo
