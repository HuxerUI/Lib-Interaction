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
  /// Clip corner radii applied to the ripple's paint clip. Set these to match the host surface's rounding;
  /// the zero default clips to the rectangular node bounds.
  CornerRadii corner_radii = {};
  /// Whether ripples expand from the node center instead of the press position.
  bool center_origin = false;
  bool operator==(const RippleStyle&) const = default;
};

/// A retained modifier that paints an expanding, fading circle from each primary press position.
///
/// Apply through View::With(). The modifier observes pointer sequences without consuming them, so sibling gesture
/// recognizers and OnClick continue to work. Circles are clipped to the node bounds and RippleStyle::corner_radii;
/// an active ripple is resolved immediately when its pointer sequence is canceled. Reduced motion resolves
/// an active ripple immediately to its settled, fully faded state.
///
/// The modifier paints only; it does not change layout, clipping, hit testing beyond making the node an input
/// target, or semantics. Applying it to a disabled node suppresses new ripples.
/// @code
/// Card{...}.With(Ripple{RippleStyle{.color = Color::Rgb(255, 255, 255, 0.14F)}});
/// @endcode
class Ripple {
public:
  /// Retained extension created by the framework when the modifier is attached through With(). Defined in the
  /// library; consumers never construct it directly.
  class Extension;

  /// Returns the retained-modifier descriptor used by View::With().
  /// @return The framework modifier descriptor; attach the value through With().
  static const detail::ModifierDescriptor& Descriptor();

  /// Validates and stores the paint configuration.
  /// @param style Ripple paint configuration.
  /// @throws std::invalid_argument If duration is non-finite or not positive, or maximum_opacity is non-finite or
  ///     outside [0, 1].
  explicit Ripple(RippleStyle style = {});

  /// The paint configuration applied by the retained extension.
  RippleStyle style;
};

} // namespace huxerui::interaction
