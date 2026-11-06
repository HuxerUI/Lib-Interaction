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
  return MaterialTheme(dark.Get() ? MaterialDarkThemeSpec() : MaterialLightThemeSpec(), Scope([=] {
    const auto& theme = UseTheme();
    return Column {
      Row {
        Column {
          Text("Interaction", TextRole::Title),
          Text("Interactive animation widgets for HuxerUI: pressables, hover lifts, pulses, and ripples.",
               TextRole::Label),
        }.With(Grow{}, Spacing(6.0F)),
        Button(dark.Get() ? "Light theme" : "Dark theme").OnClick([=] { dark = !dark.Get(); }),
      }.With(Spacing(16.0F), CrossAlign(CrossAxisAlignment::Center)),
      Row {
        Pressable(
            Column {
              Text("Press me", TextRole::Label),
              Text("Scale on hover and press", TextRole::Label),
            }.With(Spacing(4.0F), CrossAlign(CrossAxisAlignment::Center)),
            {.animation = SpringSpec{.stiffness = 320.0F, .damping_ratio = 0.7F}},
            [=] { clicks += 1; })
            .With(Frame{.width = 240.0F, .height = 120.0F},
                  Background{theme.colors.primary_container}, CornerRadius(14.0F))
            .Key("pressable-demo"),
        HoverLift(
            Column {
              Text("Hover me", TextRole::Label),
              Text("Lift on hover", TextRole::Label),
            }.With(Spacing(4.0F), CrossAlign(CrossAxisAlignment::Center)),
            {.lift = {0.0F, -6.0F}, .animation = TweenSpec{.duration = 0.2, .easing = Easing::EaseOut}})
            .With(Frame{.width = 240.0F, .height = 120.0F},
                  Background{theme.colors.secondary_container}, CornerRadius(14.0F))
            .Key("lift-demo"),
      }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Center)),
      Row {
        Pulsing(
            Text("Live", TextRole::Label).With(
                Frame{.width = 96.0F, .height = 40.0F},
                Background{theme.colors.tertiary_container}, CornerRadius(20.0F)),
            {.scale_amplitude = 0.05F, .duration = 0.9})
            .With(Frame{.width = 96.0F, .height = 40.0F})
            .Key("pulse-demo"),
        Text("Tap anywhere on the tiles",
             TextRole::Label).With(Foreground(theme.colors.on_surface_variant)),
        Text("clicks: " + std::to_string(clicks.Get()), TextRole::Label).Key("click-count"),
      }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Center)),
      Row {
        Pressable(
            Text("Ripple card", TextRole::Label).With(
                Frame{.width = 240.0F, .height = 96.0F},
                Background{theme.colors.surface_container_high},
                Ripple{RippleStyle{.color = Color::Rgb(255, 255, 255, 0.22F), .corner_radii = 14.0F}}),
            {.hovered_scale = 1.0F, .pressed_scale = 1.0F})
            .With(Frame{.width = 240.0F, .height = 96.0F}, CornerRadius(14.0F))
            .Key("ripple-demo"),
        Text("Ripple observes presses without consuming clicks.", TextRole::Label)
            .With(Foreground(theme.colors.on_surface_variant)),
      }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Center)),
      Text("Motion respects the system reduced-motion preference.", TextRole::Label)
          .With(Foreground(theme.colors.on_surface_variant)),
    }.With(Padding(24.0F), Spacing(24.0F), CrossAlign(CrossAxisAlignment::Start),
            Background{theme.colors.background});
  }));
}

} // namespace interaction_demo
