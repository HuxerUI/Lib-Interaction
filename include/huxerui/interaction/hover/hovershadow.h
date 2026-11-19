#pragma once

#include <huxerui/color.h>
#include <huxerui/geometry.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <utility>

namespace huxerui::interaction {

/// Presentation and motion configuration for pointer-reactive HoverShadow cards.
struct HoverShadowStyle {
  /// Perimeter glow or elevated shadow color when hovered (alpha scales smoothly with hover presence).
  Color color = Color::Rgb(64, 128, 255, 0.36F);
  /// Ambient shadow color when unhovered (fades out as hover presence rises).
  Color rest_color = Color::Rgb(0, 0, 0, 0.10F);
  /// Resting ambient shadow offset in logical pixels. Must be finite.
  Point rest_offset{0.0F, 2.0F};
  /// Resting ambient shadow blur radius in logical pixels. Must be finite and non-negative.
  float rest_blur = 8.0F;
  /// Resting ambient shadow spread in logical pixels. Must be finite.
  float rest_spread = 0.0F;
  /// Base hovered shadow offset before adding pointer displacement. Must be finite.
  Point hover_offset{0.0F, 2.0F};
  /// Maximum additional pointer-driven shadow offset along each axis. Must be finite and non-negative.
  Point max_pointer_offset{6.0F, 5.0F};
  /// Hovered shadow blur radius in logical pixels. Must be finite and non-negative.
  float blur_radius = 18.0F;
  /// Hovered shadow spread in logical pixels. Must be finite.
  float spread = 0.5F;
  /// Radial highlight color stroked along the card perimeter near the pointer (alpha 0 disables).
  Color border_glow_color = Color::Rgb(110, 168, 255, 0.72F);
  /// Stroke width of the perimeter border glow in logical pixels (0 disables). Must be finite and non-negative.
  float border_glow_width = 1.5F;
  /// Diameter of the pointer-centered perimeter glow in logical pixels. Must be finite and positive.
  float border_glow_diameter = 180.0F;
  /// Hover fade duration in seconds. Must be finite and non-negative.
  double fade = 0.2;
  /// Outer corner radii shared by the shadow caster and perimeter glow stroke. Every corner must be finite and >= 0.
  CornerRadii corner_radii{};

  bool operator==(const HoverShadowStyle&) const = default;
};

/// Wraps `content` in a transparent outer layer painting a pointer-position-reactive outer shadow/glow behind the card
/// and an optional radial perimeter border highlight above it.
/// @throws std::invalid_argument If any field of `style` is invalid.
View HoverShadow(View content, HoverShadowStyle style = {});

/// Pipeable adaptor for HoverShadow.
[[nodiscard]] inline ViewAdaptor HoverShadow(HoverShadowStyle style = {}) {
  return ViewAdaptor([style](View content) { return HoverShadow(std::move(content), style); });
}

} // namespace huxerui::interaction
