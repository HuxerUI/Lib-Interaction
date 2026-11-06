#pragma once

#include <huxerui/animation.h>
#include <huxerui/view.h>

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
///
/// The motion is declarative: each participating modifier animates toward its target with unbounded reverse
/// repetition, so the visible value alternates between the rest state and the target without application state.
/// The oscillation pauses under the framework's reduced-motion policy, which resolves animated values to their
/// targets immediately.
///
/// Call during composition. The widget owns no state and preserves nothing across recomposition beyond the
/// framework's modifier reconciliation.
/// @param content Content displayed inside the pulsing surface. The view imposes no intrinsic frame.
/// @param style Amplitudes, duration, curve, and participating channels. Must satisfy the documented ranges.
/// @return A View whose modifiers retain the oscillation while mounted.
/// @throws std::invalid_argument If scale_amplitude is non-finite or negative, minimum_opacity is non-finite or
///     outside [0, 1], duration is non-finite or not positive, or no channel participates.
/// @code
/// Pulsing(StatusDot(), {.scale_amplitude = 0.08F, .animate_opacity = true, .minimum_opacity = 0.4F})
///     .With(Frame{.width = 12.0F, .height = 12.0F});
/// @endcode
View Pulsing(View content, PulsingStyle style = {});

} // namespace huxerui::interaction
