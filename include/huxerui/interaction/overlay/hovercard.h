#pragma once

#include <huxerui/event.h>
#include <huxerui/geometry.h>
#include <huxerui/gesture.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/presentation.h>
#include <huxerui/view.h>

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <utility>

namespace huxerui::interaction {
namespace detail {
struct HoverCardControllerState;
} // namespace detail

/// Copyable handle for observing or programmatically showing/hiding a mounted HoverCard.
/// Default-constructed handles are unbound and safe outside an Application runtime; bind one with UseHoverCardController().
class HoverCardController {
public:
  HoverCardController() noexcept = default;

  [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(state_); }

  /// Reports whether the bound HoverCard popup is currently open (subscribes active composition).
  [[nodiscard]] bool IsOpen() const;

  /// Immediately shows the bound HoverCard when mounted.
  void Show() const;

  /// Immediately dismisses the bound HoverCard when mounted.
  void Hide() const;

  bool operator==(const HoverCardController&) const = default;

private:
  explicit HoverCardController(std::shared_ptr<detail::HoverCardControllerState> state) : state_(std::move(state)) {}

  std::shared_ptr<detail::HoverCardControllerState> state_;

  friend HoverCardController UseHoverCardController();
  friend View HoverCard(View content, std::function<View(PopupContext)> card, struct HoverCardStyle style,
                        std::function<void(bool)> on_open_changed);
};

/// Creates a HoverCardController remembered in the current composition scope.
HoverCardController UseHoverCardController();

/// Value-comparable configuration for HoverCard.
struct HoverCardStyle {
  /// Delay before a hover-enter presents the card. Must be finite and non-negative.
  std::chrono::duration<double> show_delay{0.35};
  /// Grace period before a hover-leave dismisses the card. Must be finite and non-negative.
  std::chrono::duration<double> hide_delay{0.12};
  /// Whether a touch long press presents the card.
  bool long_press = true;
  /// Touch hold duration override. Empty uses the platform long-press duration.
  std::optional<std::chrono::duration<double>> long_press_duration = std::nullopt;
  /// Preferred side and cross-axis alignment relative to the anchor.
  AnchorPlacement placement{};
  /// Distance from the anchor in logical pixels. Must be finite and non-negative.
  float gap = 4.0F;
  /// Minimum distance from viewport safe-area edges in logical pixels. Must be finite and non-negative.
  float viewport_margin = 8.0F;
  /// Additional translation from the resolved anchored position. Must be finite.
  Point offset{};
  /// Whether a pointer press outside the popup dismisses the card.
  bool dismiss_on_outside_press = true;
  /// Whether Escape or Back dismisses the card.
  bool dismiss_on_cancel = true;
  /// Whether keyboard focus is confined to the popup while visible.
  bool trap_focus = false;
  /// Keeps keyboard focus on the mounted anchor when pointer input targets popup content without its own focus target.
  bool retain_anchor_focus = false;
  /// Optional controller connected to this HoverCard instance.
  HoverCardController controller{};

  bool operator==(const HoverCardStyle&) const = default;
};

/// Presents arbitrary content as an anchored hover / touch-long-press popup card.
/// @throws std::invalid_argument If delays, geometry options, or `card` are invalid.
View HoverCard(View content, std::function<View(PopupContext)> card, HoverCardStyle style = {},
               std::function<void(bool)> on_open_changed = {});

/// Pipeable adaptor for HoverCard.
[[nodiscard]] inline ViewAdaptor HoverCard(std::function<View(PopupContext)> card, HoverCardStyle style = {},
                                           std::function<void(bool)> on_open_changed = {}) {
  return ViewAdaptor(
      [card = std::move(card), style, on_open_changed = std::move(on_open_changed)](View content) {
        return HoverCard(std::move(content), card, style, on_open_changed);
      });
}

} // namespace huxerui::interaction
