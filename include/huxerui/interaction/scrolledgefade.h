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
///
/// Attach through View::With() to the ScrollView that owns controller. The extension reads
/// ScrollController::Metrics() on NodeExtension::OnScrollActivity and paints a gradient only when the corresponding
/// edge is scrollable and has been passed. The fade follows the scroll axis automatically.
/// @code
/// auto controller = UseScrollController();
/// return ScrollView(content)
///     .Controller(controller)
///     .With(ScrollEdgeFade{controller, ScrollEdgeFadeStyle{.color = Color::White()}});
/// @endcode
class ScrollEdgeFade {
public:
  /// Retained extension created by the framework when the modifier is attached through With().
  class Extension;

  /// Returns the retained-modifier descriptor used by View::With().
  static const detail::ModifierDescriptor& Descriptor();

  /// Validates and stores the connected controller and appearance.
  /// @param controller Controller bound to the same scroll container.
  /// @param style Fade appearance.
  /// @throws std::invalid_argument If extent is invalid.
  explicit ScrollEdgeFade(ScrollController controller = ScrollController{}, ScrollEdgeFadeStyle style = {});

  /// Controller whose metrics decide whether each edge is active.
  ScrollController controller;
  /// Fade appearance.
  ScrollEdgeFadeStyle style;
};

} // namespace huxerui::interaction
