#pragma once

#include <huxerui/color.h>
#include <huxerui/geometry.h>
#include <huxerui/view.h>

#include <vector>

namespace huxerui::interaction {

/// Paint configuration for Fireworks. The default palette reads over both light and dark surfaces.
struct FireworksStyle {
  /// Particle colors, cycled with jitter across the burst. Must not be empty.
  std::vector<Color> palette = {
      Color::Rgb(255, 255, 255, 0.95F), Color::Rgb(255, 199, 88, 0.95F),
      Color::Rgb(120, 170, 255, 0.95F), Color::Rgb(255, 130, 170, 0.95F),
  };
  /// Particles per burst. Must be at least one and at most 64.
  int particle_count = 24;
  /// Seconds from launch to the last particle settling. Must be finite and positive.
  double duration = 0.9;
  /// Maximum travel distance from the burst origin, in logical pixels. Must be finite and positive.
  float spread_radius = 56.0F;
  /// Downward droop applied to particle trails as the burst ages, in logical pixels. Must be finite and
  /// non-negative.
  float gravity = 24.0F;
  /// Clip corner radii applied to the burst paint. The zero default clips to the rectangular node bounds.
  CornerRadii corner_radii = {};
  bool operator==(const FireworksStyle&) const = default;
};

/// One burst request carried by the modifier value. Bump id and set the node-local origin to spawn a new burst;
/// the framework only delivers the value to the retained extension when it differs, so equal requests are free.
struct FireworksBurst {
  /// Monotonic burst counter; zero never spawns.
  std::uint64_t id = 0;
  /// Burst origin in the host node's local logical coordinate space.
  Point local_position;

  bool operator==(const FireworksBurst&) const = default;
};

/// A retained, paint-only modifier that plays small particle bursts inside the host node.
///
/// Apply through View::With() and spawn bursts by composing the modifier value from state: when the incoming
/// FireworksBurst id differs from the last delivered one, the extension launches a radial particle burst at
/// FireworksBurst::local_position. Particles fly outward with per-particle speed, size, color, and a short
/// stagger, decelerate, droop with FireworksStyle::gravity, and fade to nothing. Bursts are bounded to three at
/// a time; older bursts drop when the bound is exceeded.
///
/// The modifier paints only; it does not change layout, clipping, hit testing, or semantics, and it does not
/// consume events. Reduced motion resolves bursts immediately to their settled, invisible state.
/// @code
/// auto burst = UseState(FireworksBurst{});
/// DoubleClickable(surface, {}, [burst](const MultiTapEvent& event) {
///     burst = FireworksBurst{.id = burst.Get().id + 1, .local_position = event.position};
///   })
///     .With(Fireworks{FireworksStyle{}, burst.Get()});
/// @endcode
class Fireworks {
public:
  /// Retained extension created by the framework when the modifier is attached through With(). Defined in the
  /// library; consumers never construct it directly.
  class Extension;

  /// Returns the retained-modifier descriptor used by View::With().
  /// @return The framework modifier descriptor; attach the value through With().
  static const detail::ModifierDescriptor& Descriptor();

  /// Validates and stores the paint configuration.
  /// @param style Fireworks paint configuration.
  /// @param burst Initial burst request; the zero id never spawns.
  /// @throws std::invalid_argument If the palette is empty, particle_count is outside [1, 64], duration or
  ///     spread_radius is non-finite or not positive, or gravity is non-finite or negative.
  explicit Fireworks(FireworksStyle style = {}, FireworksBurst burst = {});

  /// The paint configuration applied by the retained extension.
  FireworksStyle style;

  /// The burst request delivered to the retained extension.
  FireworksBurst burst;
};

} // namespace huxerui::interaction
