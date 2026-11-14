#pragma once

#include <huxerui/animation.h>
#include <huxerui/color.h>
#include <huxerui/geometry.h>
#include <huxerui/view.h>

namespace huxerui::interaction {

/// Paint configuration for Shimmer. The default highlight reads over both light and dark placeholder surfaces.
struct ShimmerStyle {
  /// Highlight color at the band center. The band fades to transparent at both edges.
  Color color = Color::Rgb(255, 255, 255, 0.32F);
  /// Seconds for one left-to-right sweep. Must be finite and positive.
  double period = 1.6;
  /// Band width as a fraction of the node width. Must be finite, positive, and at most one.
  float band_width = 0.35F;
  /// Clip corner radii applied to the highlight paint. Set these to match the host surface's rounding;
  /// the zero default clips to the rectangular node bounds.
  CornerRadii corner_radii = {};
  bool operator==(const ShimmerStyle&) const = default;
};

/// A retained modifier that paints a looping highlight sweep over placeholder content.
///
/// Apply through View::With(). The sweep runs continuously while the modifier is mounted: one highlight band
/// crosses the node left to right over ShimmerStyle::period and restarts. The band is clipped to the node bounds
/// and ShimmerStyle::corner_radii; reduced motion parks the sweep outside the node so the content stays plain.
///
/// The modifier paints only; it does not change layout, clipping, hit testing, or semantics, and it involves no
/// application state and no recomposition.
/// @code
/// Card{...}.With(Shimmer{ShimmerStyle{.corner_radii = 12.0F}});
/// @endcode
class Shimmer {
public:
  /// Retained extension created by the framework when the modifier is attached through With(). Defined in the
  /// library; consumers never construct it directly.
  class Extension;

  /// Returns the retained-modifier descriptor used by View::With().
  /// @return The framework modifier descriptor; attach the value through With().
  static const detail::ModifierDescriptor& Descriptor();

  /// Validates and stores the paint configuration.
  /// @param style Shimmer paint configuration.
  /// @throws std::invalid_argument If period is non-finite or not positive, or band_width is non-finite, not
  ///     positive, or above one.
  explicit Shimmer(ShimmerStyle style = {});

  /// The paint configuration applied by the retained extension.
  ShimmerStyle style;
};

} // namespace huxerui::interaction
