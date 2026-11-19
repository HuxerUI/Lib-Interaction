#pragma once

#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <utility>

namespace huxerui::interaction {
namespace detail {
struct RovingFocusControllerState;
} // namespace detail

struct RovingFocusStyle;

/// Selects which directional arrow keys navigate between items in a RovingFocusGroup.
enum class RovingFocusOrientation {
  /// ArrowLeft / ArrowRight navigate.
  Horizontal,
  /// ArrowUp / ArrowDown navigate.
  Vertical,
  /// Both horizontal and vertical arrows navigate.
  Both,
};

/// Copyable handle for observing the active item index and programmatically shifting focus within a RovingFocusGroup.
/// Default-constructed handles are unbound and safe outside an Application runtime; bind one with
/// UseRovingFocusController().
class RovingFocusController {
public:
  RovingFocusController() noexcept = default;

  [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(state_); }

  /// Currently active/focused item index within the group (subscribes active composition).
  [[nodiscard]] std::size_t ActiveIndex() const;

  /// Requests focus on the registered item at `index`. Returns true if focus moved successfully.
  bool FocusIndex(std::size_t index) const;
  /// Moves focus to the next eligible item in ascending index order.
  bool FocusNext() const;
  /// Moves focus to the previous eligible item in descending index order.
  bool FocusPrevious() const;
  /// Moves focus to the first eligible item in ascending index order.
  bool FocusFirst() const;
  /// Moves focus to the last eligible item in ascending index order.
  bool FocusLast() const;

  bool operator==(const RovingFocusController&) const = default;

private:
  explicit RovingFocusController(std::shared_ptr<detail::RovingFocusControllerState> state)
      : state_(std::move(state)) {}

  std::shared_ptr<detail::RovingFocusControllerState> state_;

  friend RovingFocusController UseRovingFocusController(std::size_t initial_index);
  friend View RovingFocusGroup(View content, RovingFocusStyle style, std::function<void(std::size_t)> on_active_changed);
  friend View RovingFocusItem(View content, std::size_t index, struct RovingFocusItemStyle style);
};

/// Remembers a RovingFocusController in the current composition scope.
RovingFocusController UseRovingFocusController(std::size_t initial_index = 0);

/// Configuration for RovingFocusGroup.
struct RovingFocusStyle {
  /// Arrow directions handled by the group.
  RovingFocusOrientation orientation = RovingFocusOrientation::Horizontal;
  /// Wraps from last to first (and vice versa) when true.
  bool loop = true;
  /// Handles Home / End keys to jump to the first / last eligible item when true.
  bool home_end = true;
  /// Optional external controller bound to this group.
  RovingFocusController controller{};

  bool operator==(const RovingFocusStyle&) const = default;
};

/// Configuration for RovingFocusItem.
struct RovingFocusItemStyle {
  /// Attaches `Focusable{true}` to the wrapped content when true (useful for non-button containers/cards).
  bool focusable = false;

  bool operator==(const RovingFocusItemStyle&) const = default;
};

/// Provides a roving-focus scope to descendant RovingFocusItem nodes and intercepts directional keys.
View RovingFocusGroup(View content, RovingFocusStyle style = {}, std::function<void(std::size_t)> on_active_changed = {});

/// Pipeable adaptor for RovingFocusGroup.
[[nodiscard]] inline ViewAdaptor RovingFocusGroup(RovingFocusStyle style = {},
                                                  std::function<void(std::size_t)> on_active_changed = {}) {
  return ViewAdaptor([style, on_active_changed = std::move(on_active_changed)](View content) {
    return RovingFocusGroup(std::move(content), style, on_active_changed);
  });
}

/// Registers a child view at `index` inside the enclosing RovingFocusGroup with focus targeting and focus observation.
View RovingFocusItem(View content, std::size_t index, RovingFocusItemStyle style = {});

/// Pipeable adaptor for RovingFocusItem.
[[nodiscard]] inline ViewAdaptor RovingFocusItem(std::size_t index, RovingFocusItemStyle style = {}) {
  return ViewAdaptor([index, style](View content) { return RovingFocusItem(std::move(content), index, style); });
}

} // namespace huxerui::interaction
