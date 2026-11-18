#pragma once

#include <huxerui/animation.h>
#include <huxerui/event.h>
#include <huxerui/geometry.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <functional>
#include <memory>
#include <utility>

namespace huxerui::interaction {
namespace detail {
struct SwipeControllerState;
} // namespace detail

/// Copyable handle for observing or programmatically opening/closing a mounted SwipeActions row.
/// Default-constructed handles are unbound and safe outside an Application runtime; bind one with UseSwipeController().
class SwipeController {
public:
  SwipeController() noexcept = default;

  [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(state_); }

  /// True when the action strip is settled in the open state (subscribes active composition).
  [[nodiscard]] bool IsOpen() const;
  /// Current exposed extent in logical pixels (subscribes active composition).
  [[nodiscard]] float Offset() const;
  /// Current normalized reveal fraction in [0, 1] (subscribes active composition).
  [[nodiscard]] float Progress() const;

  /// Settles the connected row open to its configured `action_extent`.
  void Open() const;
  /// Settles the connected row closed.
  void Close() const;
  /// Toggles between open and closed.
  void Toggle() const;

  bool operator==(const SwipeController&) const = default;

private:
  explicit SwipeController(std::shared_ptr<detail::SwipeControllerState> state) : state_(std::move(state)) {}

  std::shared_ptr<detail::SwipeControllerState> state_;

  friend SwipeController UseSwipeController(bool initially_open);
  friend View SwipeActions(View content, std::function<View(struct SwipeActionContext)> actions,
                           struct SwipeActionsStyle style, std::function<void(bool)> on_open_changed);
};

/// Remembers a SwipeController in the current composition scope.
SwipeController UseSwipeController(bool initially_open = false);

/// Context passed to SwipeActions' trailing-strip builder.
/// Implicitly converts to `std::function<void()>` (`close`), so builders taking either `SwipeActionContext` or
/// `std::function<void()>` work seamlessly.
struct SwipeActionContext {
  /// Settles the action strip closed.
  std::function<void()> close;
  /// Normalized reveal fraction in [0, 1] for driving action icon scale or opacity.
  float progress = 0.0F;

  operator std::function<void()>() const { return close; }
};

/// Presentation and threshold configuration for SwipeActions.
struct SwipeActionsStyle {
  /// Width of the trailing action strip revealed by a leftward swipe, in logical pixels. Must be finite and positive.
  float action_extent = 88.0F;
  /// Fraction of action_extent that must be exposed on release before the strip settles open. Must be in [0, 1].
  float activation_ratio = 0.45F;
  /// Leftward release velocity, in logical pixels per second, that opens the strip below the distance threshold.
  float minimum_velocity = 400.0F;
  /// Motion used while the pointer drags the content.
  AnimationSpec follow_motion = TweenSpec{.duration = 0.04, .easing = Easing::Linear};
  /// Motion used when the content settles open or closed.
  AnimationSpec settle_motion = TweenSpec{.duration = 0.2, .easing = Easing::EaseOut};
  /// Optional controller connected to this SwipeActions instance.
  SwipeController controller{};

  bool operator==(const SwipeActionsStyle&) const = default;
};

/// Composes content with a trailing action strip revealed by a horizontal swipe.
/// @throws std::invalid_argument If `style` is invalid or `actions` is empty.
View SwipeActions(View content, std::function<View(SwipeActionContext)> actions,
                  SwipeActionsStyle style = {}, std::function<void(bool)> on_open_changed = {});

/// Pipeable adaptor for SwipeActions.
[[nodiscard]] inline ViewAdaptor SwipeActions(std::function<View(SwipeActionContext)> actions,
                                              SwipeActionsStyle style = {},
                                              std::function<void(bool)> on_open_changed = {}) {
  return ViewAdaptor(
      [actions = std::move(actions), style, on_open_changed = std::move(on_open_changed)](View content) {
        return SwipeActions(std::move(content), actions, style, on_open_changed);
      });
}

} // namespace huxerui::interaction
