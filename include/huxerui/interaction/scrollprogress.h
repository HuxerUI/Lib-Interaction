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
///
/// Attach through View::With() to the ScrollView that owns controller. The extension listens to
/// NodeExtension::OnScrollActivity and reads ScrollController::Metrics(); it paints nothing when the controller is
/// disconnected or the content fits the viewport. The bar follows the scroll axis automatically.
/// @code
/// auto controller = UseScrollController();
/// return ScrollView(content)
///     .Controller(controller)
///     .With(ScrollProgress{controller, ScrollProgressStyle{}});
/// @endcode
class ScrollProgress {
public:
  /// Retained extension created by the framework when the modifier is attached through With().
  class Extension;

  /// Returns the retained-modifier descriptor used by View::With().
  static const detail::ModifierDescriptor& Descriptor();

  /// Validates and stores the connected controller and appearance.
  /// @param controller Controller bound to the same scroll container.
  /// @param style Bar appearance.
  /// @throws std::invalid_argument If thickness or inset is invalid.
  explicit ScrollProgress(ScrollController controller = ScrollController{}, ScrollProgressStyle style = {});

  /// Controller whose metrics supply the current position and extent.
  ScrollController controller;
  /// Bar appearance.
  ScrollProgressStyle style;
};

} // namespace huxerui::interaction
