#pragma once

#include <huxerui/animation.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <utility>

namespace huxerui::interaction {

/// Playback configuration for Pulsing. Defaults produce a slow breathing scale without opacity motion.
struct PulsingStyle {
  /// Peak scale above the rest state of 1.0. Zero disables scale motion when animate_scale is true.
  float scale_amplitude = 0.04F;
  /// Lower opacity bound; opacity oscillates between this value and 1.0 when animate_opacity is true.
  float minimum_opacity = 0.55F;
  /// Seconds spent moving in one direction, so one full there-and-back cycle takes twice this value.
  double duration = 1.1;
  /// Curve applied within each direction of the oscillation.
  Easing easing = Easing::EaseInOut;
  /// Whether the scale participates in the oscillation.
  bool animate_scale = true;
  /// Whether the opacity participates in the oscillation.
  bool animate_opacity = false;

  bool operator==(const PulsingStyle&) const = default;
};

/// Composes content whose scale or opacity oscillates continuously between the rest and target values.
/// @throws std::invalid_argument If amplitudes, opacity, duration, or channel selection are invalid.
View Pulsing(View content, PulsingStyle style = {});

/// Pipeable adaptor for Pulsing.
[[nodiscard]] inline ViewAdaptor Pulsing(PulsingStyle style = {}) {
  return ViewAdaptor([style](View content) { return Pulsing(std::move(content), style); });
}

} // namespace huxerui::interaction
