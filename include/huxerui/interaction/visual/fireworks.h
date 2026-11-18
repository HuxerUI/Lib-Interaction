#pragma once

#include <huxerui/color.h>
#include <huxerui/geometry.h>
#include <huxerui/view.h>

#include <cstdint>
#include <vector>

namespace huxerui::interaction {

/// Paint configuration for Fireworks.
struct FireworksStyle {
  /// Particle colors, cycled with jitter across the burst. Must not be empty.
  std::vector<Color> palette = {
      Color::Rgb(255, 255, 255, 0.95F), Color::Rgb(255, 199, 88, 0.95F),
      Color::Rgb(120, 170, 255, 0.95F), Color::Rgb(255, 130, 170, 0.95F),
  };
  /// Particles per burst. Must be in [1, 64].
  int particle_count = 24;
  /// Seconds from launch to the last particle settling. Must be finite and positive.
  double duration = 0.9;
  /// Maximum travel distance from the burst origin, in logical pixels. Must be finite and positive.
  float spread_radius = 56.0F;
  /// Downward droop applied to particle trails as the burst ages, in logical pixels. Must be finite and non-negative.
  float gravity = 24.0F;
  /// Clip corner radii applied to the burst paint.
  CornerRadii corner_radii = {};

  bool operator==(const FireworksStyle&) const = default;
};

/// One burst request carried by the Fireworks modifier value.
struct FireworksBurst {
  /// Monotonic burst counter; zero never spawns.
  std::uint64_t id = 0;
  /// Burst origin in the host node's local logical coordinate space.
  Point local_position;

  bool operator==(const FireworksBurst&) const = default;
};

/// A retained, paint-only modifier that plays small particle bursts inside the host node.
class Fireworks {
public:
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  /// Validates and stores the paint configuration and burst request.
  /// @throws std::invalid_argument If palette, particle_count, duration, spread_radius, or gravity is invalid.
  explicit Fireworks(FireworksStyle style = {}, FireworksBurst burst = {});

  FireworksStyle style;
  FireworksBurst burst;

  bool operator==(const Fireworks&) const = default;
};

} // namespace huxerui::interaction
