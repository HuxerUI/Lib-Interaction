#include <huxerui/interaction/visual/pulse.h>

#include "../internal/validate.h"

#include <stdexcept>
#include <utility>

namespace huxerui::interaction {
namespace {

constexpr AnimationPlayback Loop() {
  return AnimationPlayback{.delay = 0.0, .iterations = std::nullopt, .repeat_mode = RepeatMode::Reverse};
}

void ValidateStyle(const PulsingStyle& style) {
  internal::RequireNonNegative(style.scale_amplitude,
                               "HuxerUI Interaction: Pulsing scale amplitude must be finite and non-negative");
  internal::RequireUnitInterval(style.minimum_opacity,
                                "HuxerUI Interaction: Pulsing minimum opacity must be finite and in [0, 1]");
  internal::RequirePositive(style.duration, "HuxerUI Interaction: Pulsing duration must be finite and positive");
  if(!style.animate_scale && !style.animate_opacity) {
    throw std::invalid_argument("HuxerUI Interaction: Pulsing requires at least one participating channel");
  }
}

} // namespace

View Pulsing(View content, PulsingStyle style) {
  ValidateStyle(style);
  const TweenSpec motion{.duration = style.duration, .easing = style.easing};
  if(style.animate_scale) {
    content = std::move(content).With(Scale(AnimateTo(1.0F + style.scale_amplitude, motion, Loop())));
  }
  if(style.animate_opacity) {
    content = std::move(content).With(Opacity(AnimateTo(style.minimum_opacity, motion, Loop())));
  }
  return content;
}

} // namespace huxerui::interaction
