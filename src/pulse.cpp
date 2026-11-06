#include <huxerui/interaction/pulse.h>

#include <cmath>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {
namespace {

constexpr AnimationPlayback Loop() {
  return AnimationPlayback{.delay = 0.0, .iterations = std::nullopt, .repeat_mode = RepeatMode::Reverse};
}

void ValidateStyle(const PulsingStyle& style) {
  if(!std::isfinite(style.scale_amplitude) || style.scale_amplitude < 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: Pulsing scale amplitude must be finite and non-negative");
  }
  if(!std::isfinite(style.minimum_opacity) || style.minimum_opacity < 0.0F || style.minimum_opacity > 1.0F) {
    throw std::invalid_argument("HuxerUI Interaction: Pulsing minimum opacity must be finite and in [0, 1]");
  }
  if(!std::isfinite(style.duration) || style.duration <= 0.0) {
    throw std::invalid_argument("HuxerUI Interaction: Pulsing duration must be finite and positive");
  }
  if(!style.animate_scale && !style.animate_opacity) {
    throw std::invalid_argument("HuxerUI Interaction: Pulsing requires at least one participating channel");
  }
}

} // namespace

View Pulsing(View content, PulsingStyle style) {
  ValidateStyle(style);
  const TweenSpec motion{.duration = style.duration, .easing = style.easing};
  if(style.animate_scale) {
    content = std::move(content).With(Scale(AnimateTo(
        1.0F + style.scale_amplitude, motion, Loop())));
  }
  if(style.animate_opacity) {
    content = std::move(content).With(Opacity(AnimateTo(
        style.minimum_opacity, motion, Loop())));
  }
  return std::move(content);
}

} // namespace huxerui::interaction
