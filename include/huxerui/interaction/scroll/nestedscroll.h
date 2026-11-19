#pragma once

#include <huxerui/geometry.h>
#include <huxerui/modifier.h>
#include <huxerui/scroll.h>

#include <functional>
#include <memory>
#include <utility>

namespace huxerui::interaction {
namespace detail {
struct ScrollHideControllerState;
} // namespace detail

/// User callbacks forwarded by NestedScrollConnection during nested scroll and fling transactions.
/// Each callback returns how much of `available` (or `available_velocity`) it consumed; values outside the valid
/// range are clamped safely to match `available` before reporting back to HuxerUI.
struct NestedScrollCallbacks {
  std::function<float(Axis axis, float available, ScrollSource source)> on_pre_scroll{};
  std::function<float(Axis axis, float consumed, float available, ScrollSource source)> on_post_scroll{};
  std::function<float(Axis axis, float available_velocity)> on_pre_fling{};
  std::function<float(Axis axis, float consumed_velocity, float available_velocity)> on_post_fling{};
};

/// A retained modifier attaching nested-scroll and fling interception hooks to a scrollable View.
class NestedScrollConnection {
public:
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  NestedScrollConnection() noexcept = default;
  explicit NestedScrollConnection(NestedScrollCallbacks callbacks)
      : callbacks(std::make_shared<const NestedScrollCallbacks>(std::move(callbacks))) {}
  explicit NestedScrollConnection(std::shared_ptr<const NestedScrollCallbacks> callbacks) noexcept
      : callbacks(std::move(callbacks)) {}

  std::shared_ptr<const NestedScrollCallbacks> callbacks;

  bool operator==(const NestedScrollConnection&) const = default;
};

/// Behavior configuration for UseScrollHideController().
struct ScrollHideStyle {
  /// Maximum hidden travel in logical pixels. Must be finite and positive.
  float extent = 56.0F;
  /// Scroll axis observed by the attached NestedScrollConnection.
  Axis axis = Axis::Vertical;
  /// When true, scroll deltas used to hide/reveal the chrome are consumed before the scroll container moves
  /// (collapsing header style). When false, scroll deltas are observed without consumption (floating bar / FAB style).
  bool consume_scroll = true;

  bool operator==(const ScrollHideStyle&) const = default;
};

/// Copyable handle synchronizing collapsible chrome or floating bars with a scroll container via
/// NestedScrollConnection. Default-constructed handles are unbound and safe outside an Application runtime; bind
/// one with UseScrollHideController().
class ScrollHideController {
public:
  ScrollHideController() noexcept = default;

  [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(state_); }

  /// Returns a NestedScrollConnection to attach to the driving ScrollView / scrollable View.
  [[nodiscard]] NestedScrollConnection Connection() const;

  /// Current hidden displacement in `[0, extent]` logical pixels (subscribes active composition).
  [[nodiscard]] float HiddenOffset() const;

  /// Current normalized hidden fraction in `[0, 1]` (subscribes active composition).
  [[nodiscard]] float Fraction() const;

  /// True when the bar has reached its configured hidden extent (subscribes active composition).
  [[nodiscard]] bool IsHidden() const;

  /// Resets the hidden offset to `0` (fully visible).
  void Show() const;

  /// Advances the hidden offset to `extent` (fully hidden).
  void Hide() const;

  bool operator==(const ScrollHideController&) const = default;

private:
  explicit ScrollHideController(std::shared_ptr<detail::ScrollHideControllerState> state) : state_(std::move(state)) {}

  std::shared_ptr<detail::ScrollHideControllerState> state_;

  friend ScrollHideController UseScrollHideController(ScrollHideStyle style);
};

/// Remembers a ScrollHideController in the current composition scope.
/// @throws std::invalid_argument If `style.extent` is non-finite or not positive.
ScrollHideController UseScrollHideController(ScrollHideStyle style = {});

} // namespace huxerui::interaction
