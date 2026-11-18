#include "app.h"
#include "demos/demos.h"

#include <huxerui/environment.h>
#include <huxerui/vector.h>

#include <utility>

namespace interaction_demo {
using namespace huxerui;
using namespace huxerui::interaction;

namespace {

enum class ThemeMode { System, Light, Dark };

const StrokeStyle kIconStroke{.width = 1.6F, .cap = StrokeCap::Round, .join = StrokeJoin::Round};

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
  auto mode = UseState(ThemeMode::System);
  const bool dark = mode.Get() == ThemeMode::Dark
                    || (mode.Get() == ThemeMode::System && UseSystemThemeBrightness() == ThemeBrightness::Dark);
  DemoState state{
      .clicks = UseState(0),
      .holds = UseState(0),
      .doubles = UseState(0),
  };
  return MaterialTheme(BlueThemeSpec(dark), Scope([=] {
    const auto& theme = UseTheme();
    const ViewportClass viewport = UseViewportClass();
    const bool compact = viewport == ViewportClass::Compact;
    const bool expanded = viewport == ViewportClass::Expanded;
    const DemoContext context{theme.colors, compact, expanded, dark};
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
          Text("Headless interaction primitives, controllers, and visual feedback for HuxerUI.",
               TextRole::Label),
        }.With(Grow{}, Spacing(6.0F)),
        theme_button(ThemeSystemIcon(), "Follow system theme", ThemeMode::System).Key("theme-system"),
        theme_button(ThemeLightIcon(), "Light theme", ThemeMode::Light).Key("theme-light"),
        theme_button(ThemeDarkIcon(), "Dark theme", ThemeMode::Dark).Key("theme-dark"),
      }.With(Spacing(4.0F), CrossAlign(CrossAxisAlignment::Center)),
      BloomSection(context),
      PressSection(state, context),
      PaintSection(context),
      GestureSection(state, context),
      ReorderSection(context),
      LayoutSection(context),
      DragSection(context),
      StatusSection(state, context),
    }.With(Padding(compact ? 16.0F : 24.0F), Spacing(20.0F), CrossAlign(CrossAxisAlignment::Stretch)))
        .With(Background{theme.colors.background}, ScrollBar())
        .Key("gallery");
  }));
}

} // namespace interaction_demo
