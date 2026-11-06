#include "app.h"
#include <huxerui/interaction.h>

#include <string>

namespace interaction_demo {
using namespace huxerui;
using namespace huxerui::interaction;

[[huxerui::composable]]
View PreviewApp() {
  auto clicks = UseState(0);
  auto dark = UseState(false);
  auto shake = UseShake();
  return MaterialTheme(dark.Get() ? MaterialDarkThemeSpec() : MaterialLightThemeSpec(), Scope([=] {
    const auto& theme = UseTheme();
    const auto surface = theme.colors.surface_container_high;
    const auto tile = Frame{.width = 240.0F, .height = 120.0F};
    return Column {
      Row {
        Column {
          Text("Interaction", TextRole::Title),
          Text("Interactive animation widgets for HuxerUI: presses, hovers, pulses, ripples, drags, and shakes.",
               TextRole::Label),
        }.With(Grow{}, Spacing(6.0F)),
        Button(dark.Get() ? "Light theme" : "Dark theme").OnClick([=] { dark = !dark.Get(); }),
      }.With(Spacing(16.0F), CrossAlign(CrossAxisAlignment::Center)),
      Text("Press and hover", TextRole::Label).With(Foreground(theme.colors.on_surface_variant)),
      Row {
        Pressable(
            Column {
              Text("Press me", TextRole::Label),
              Text("Spring scale on hover and press", TextRole::Label),
            }.With(Spacing(4.0F), CrossAlign(CrossAxisAlignment::Center)),
            {.animation = SpringSpec{.stiffness = 320.0F, .damping_ratio = 0.7F}},
            [=] { clicks += 1; })
            .With(tile, Background{theme.colors.primary_container}, CornerRadius(14.0F))
            .Key("pressable-demo"),
        HoverLift(
            Column {
              Text("Hover me", TextRole::Label),
              Text("Lift on hover", TextRole::Label),
            }.With(Spacing(4.0F), CrossAlign(CrossAxisAlignment::Center)),
            {.lift = {0.0F, -6.0F}, .animation = TweenSpec{.duration = 0.2, .easing = Easing::EaseOut}})
            .With(tile, Background{theme.colors.secondary_container}, CornerRadius(14.0F))
            .Key("lift-demo"),
        Column {
          Shaken(
              Text("Shake me", TextRole::Label).With(
                  Frame{.width = 240.0F, .height = 72.0F},
                  Background{theme.colors.error}, CornerRadius(14.0F)),
              shake)
              .Key("shake-demo"),
          Button("Trigger shake").OnClick([shake] { shake.Trigger(); }).Key("shake-trigger"),
        }.With(Spacing(8.0F), CrossAlign(CrossAxisAlignment::Center)),
      }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Center)),
      Text("Paint and pulse", TextRole::Label).With(Foreground(theme.colors.on_surface_variant)),
      Row {
        Pulsing(
            Text("Live", TextRole::Label).With(
                Frame{.width = 96.0F, .height = 40.0F},
                Background{theme.colors.tertiary_container}, CornerRadius(20.0F)),
            {.scale_amplitude = 0.05F, .duration = 0.9})
            .With(Frame{.width = 96.0F, .height = 40.0F})
            .Key("pulse-demo"),
        Text("Ripple card", TextRole::Label).With(
                Frame{.width = 240.0F, .height = 96.0F}, Background{surface},
                Ripple{RippleStyle{.color = Color::Rgb(255, 255, 255, 0.22F), .corner_radii = 14.0F}})
            .With(CornerRadius(14.0F))
            .Key("ripple-demo"),
        Text("Spotlight", TextRole::Label).With(
                Frame{.width = 240.0F, .height = 96.0F}, Background{surface},
                Spotlight{SpotlightStyle{.corner_radii = 14.0F}})
            .Key("spotlight-demo"),
        Text("Hover row", TextRole::Label).With(
                Frame{.width = 240.0F, .height = 96.0F}, Background{surface},
                HoverHighlight{HoverHighlightStyle{.corner_radii = 14.0F}}),
      }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Center)),
      Text("Drag", TextRole::Label).With(Foreground(theme.colors.on_surface_variant)),
      DragSpring(
          Text("Drag me", TextRole::Label).With(
              Frame{.width = 140.0F, .height = 48.0F},
              Background{theme.colors.primary}, CornerRadius(12.0F)))
          .With(Frame{.width = 560.0F, .height = 72.0F},
                Background{theme.colors.surface_container_low}, CornerRadius(14.0F))
          .Key("dragspring-demo"),
      Row {
        Text("Tap anywhere on the tiles",
             TextRole::Label).With(Foreground(theme.colors.on_surface_variant)),
        Text("clicks: " + std::to_string(clicks.Get()), TextRole::Label).Key("click-count"),
        Text("Motion respects the system reduced-motion preference.", TextRole::Label)
            .With(Foreground(theme.colors.on_surface_variant)),
      }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Center)),
    }.With(Padding(24.0F), Spacing(20.0F), CrossAlign(CrossAxisAlignment::Start),
            Background{theme.colors.background});
  }));
}

} // namespace interaction_demo
