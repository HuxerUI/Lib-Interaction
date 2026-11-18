#pragma once

#include <huxerui/color.h>
#include <huxerui/modifier.h>
#include <huxerui/scroll.h>
#include <huxerui/view.h>

namespace huxerui::interaction {

/// Appearance for ScrollEdgeFade.
struct ScrollEdgeFadeStyle {
  /// Color faded from the scrollable edge toward the content. Use the surface background color.
  Color color = Color::Rgb(255, 255, 255, 1.0F);
  /// Fade length in logical pixels. Must be finite and positive.
  float extent = 28.0F;
  /// Whether the leading edge fades when scrolled away from the start.
  bool fade_start = true;
  /// Whether the trailing edge fades when more content remains.
  bool fade_end = true;

  bool operator==(const ScrollEdgeFadeStyle&) const = default;
};

/// Paints translucent edge fades over a connected scroll container.
class ScrollEdgeFade {
public:
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  /// Validates and stores the connected controller and appearance.
  /// @throws std::invalid_argument If extent is non-finite or not positive.
  explicit ScrollEdgeFade(ScrollController controller = ScrollController{}, ScrollEdgeFadeStyle style = {});

  ScrollController controller;
  ScrollEdgeFadeStyle style;

  bool operator==(const ScrollEdgeFade&) const = default;
};

} // namespace huxerui::interaction
