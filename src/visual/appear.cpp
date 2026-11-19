#include <huxerui/interaction/visual/appear.h>

#include "../internal/validate.h"

#include <huxerui/lifecycle.h>
#include <huxerui/state.h>

#include <utility>
#include <variant>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const AppearStyle& style) {
  internal::RequireUnitInterval(style.initial_opacity,
                                "HuxerUI Interaction: Appear initial opacity must be in [0, 1]");
  internal::RequireFinite(style.initial_offset,
                          "HuxerUI Interaction: Appear initial offset must be finite");
  internal::RequirePositive(style.initial_scale,
                            "HuxerUI Interaction: Appear initial scale must be finite and positive");
  internal::RequireFinite(style.origin.x, "HuxerUI Interaction: Appear transform origin must be finite");
  internal::RequireFinite(style.origin.y, "HuxerUI Interaction: Appear transform origin must be finite");
  internal::RequireNonNegative(style.delay, "HuxerUI Interaction: Appear delay must be finite and non-negative");
  if(const auto* tween = std::get_if<TweenSpec>(&style.motion)) {
    internal::RequireNonNegative(tween->duration,
                                 "HuxerUI Interaction: Appear tween duration must be finite and non-negative");
  } else if(const auto* spring = std::get_if<SpringSpec>(&style.motion)) {
    internal::RequirePositive(spring->stiffness,
                              "HuxerUI Interaction: Appear spring stiffness must be finite and positive");
    internal::RequireNonNegative(spring->damping_ratio,
                                 "HuxerUI Interaction: Appear spring damping ratio must be finite and non-negative");
  }
}

} // namespace

View Appear(View content, bool visible, AppearStyle style) {
  ValidateStyle(style);
  return Scope([content = std::move(content), visible, style]() -> View {
    auto mounted = UseState(false);
    Lifecycle([mounted] { mounted = true; });
    const bool active = visible && (!style.animate_on_mount || mounted.Get());
    const float target = active ? 1.0F : 0.0F;
    const AnimationPlayback playback{
        .delay = active ? style.delay : 0.0,
        .iterations = 1,
        .repeat_mode = RepeatMode::Restart,
    };

    View animated = content;
    return std::move(animated).With(
        Transition{AnimateTo(target, style.motion, playback)}
            .Opacity(style.initial_opacity, 1.0F)
            .Offset(style.initial_offset, Point{})
            .Scale(style.initial_scale, 1.0F, style.origin));
  });
}

View Appear(View content, AppearStyle style) {
  return Appear(std::move(content), true, std::move(style));
}

} // namespace huxerui::interaction
