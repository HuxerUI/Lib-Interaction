#pragma once

#include <huxerui/color.h>
#include <huxerui/geometry.h>
#include <huxerui/scroll.h>
#include <huxerui/view.h>

namespace huxerui::interaction {

/// Appearance for ScrollProgress.
struct ScrollProgressStyle {
  /// Track color drawn along the full scroll extent.
  Color track_color = Color::Rgb(0, 0, 0, 0.10F);
  /// Filled thumb color.
  Color fill_color = Color::Rgb(31, 111, 235, 0.9F);
  /// Bar thickness in logical pixels. Must be finite and positive.
  float thickness = 3.0F;
  /// Inset from the viewport edges in logical pixels. Must be finite and non-negative.
  float inset = 3.0F;
  /// Corner radii applied to the track and thumb.
  CornerRadii corner_radii{2.0F};

  bool operator==(const ScrollProgressStyle&) const = default;
};

/// Paints a scrollbar-like progress thumb over a connected scroll container.
class ScrollProgress {
public:
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  /// Validates and stores the connected controller and appearance.
  /// @throws std::invalid_argument If thickness or inset is invalid.
  explicit ScrollProgress(ScrollController controller = ScrollController{}, ScrollProgressStyle style = {});

  ScrollController controller;
  ScrollProgressStyle style;

  bool operator==(const ScrollProgress&) const = default;
};

} // namespace huxerui::interaction
