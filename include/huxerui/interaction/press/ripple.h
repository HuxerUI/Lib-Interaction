#pragma once

#include <huxerui/animation.h>
#include <huxerui/color.h>
#include <huxerui/event.h>
#include <huxerui/geometry.h>
#include <huxerui/view.h>

namespace huxerui::interaction {

/// Paint configuration for Ripple. The default tint suits both light and dark themes at low alpha.
struct RippleStyle {
  /// Circle color. The alpha channel is multiplied by the fade envelope while the ripple expands.
  Color color = Color::Rgb(0, 0, 0, 0.20F);
  /// Seconds from a primary press to a fully expanded, fully faded circle. Must be positive.
  double duration = 0.45;
  /// Multiplier applied to the color alpha at the ripple's peak. Must be finite and in [0, 1].
  float maximum_opacity = 1.0F;
  /// Clip corner radii applied to the ripple's paint clip.
  CornerRadii corner_radii = {};
  /// Whether ripples expand from the node center instead of the press position.
  bool center_origin = false;

  bool operator==(const RippleStyle&) const = default;
};

/// A retained modifier that paints an expanding, fading circle from each primary press position.
class Ripple {
public:
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  /// Validates and stores the paint configuration.
  /// @throws std::invalid_argument If duration is non-finite or not positive, or maximum_opacity is outside [0, 1].
  explicit Ripple(RippleStyle style = {});

  RippleStyle style;

  bool operator==(const Ripple&) const = default;
};

} // namespace huxerui::interaction
