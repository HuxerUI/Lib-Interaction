#pragma once

#include <huxerui/animation.h>
#include <huxerui/color.h>
#include <huxerui/geometry.h>
#include <huxerui/view.h>

namespace huxerui::interaction {

/// Paint configuration for Shimmer.
struct ShimmerStyle {
  /// Highlight color at the band center. The band fades to transparent at both edges.
  Color color = Color::Rgb(255, 255, 255, 0.32F);
  /// Seconds for one left-to-right sweep. Must be finite and positive.
  double period = 1.6;
  /// Band width as a fraction of the node width. Must be finite, positive, and at most one.
  float band_width = 0.35F;
  /// Clip corner radii applied to the highlight paint.
  CornerRadii corner_radii = {};

  bool operator==(const ShimmerStyle&) const = default;
};

/// A retained modifier that paints a looping highlight sweep over placeholder content.
class Shimmer {
public:
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  /// Validates and stores the paint configuration.
  /// @throws std::invalid_argument If period or band_width is invalid.
  explicit Shimmer(ShimmerStyle style = {});

  ShimmerStyle style;

  bool operator==(const Shimmer&) const = default;
};

} // namespace huxerui::interaction
