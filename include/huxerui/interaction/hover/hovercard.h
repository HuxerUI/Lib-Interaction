#pragma once

#include <huxerui/event.h>
#include <huxerui/gesture.h>
#include <huxerui/presentation.h>
#include <huxerui/view.h>

#include <chrono>
#include <functional>
#include <optional>

namespace huxerui::interaction {

/// Configuration for HoverCard.
struct HoverCardStyle {
  /// Delay before a hover-enter presents the card.
  std::chrono::duration<double> show_delay{0.35};
  /// Grace period before a hover-leave dismisses the card.
  std::chrono::duration<double> hide_delay{0.12};
  /// Whether a touch long press presents the card.
  bool long_press = true;
  /// Touch hold duration override. Empty uses the platform long-press duration.
  std::optional<std::chrono::duration<double>> long_press_duration = std::nullopt;
  /// Popup placement and dismissal policy.
  PopupOptions popup_options{};
};

/// Presents arbitrary content as a hover/focus-adjacent card.
///
/// Hover enter schedules a popup after show_delay; leave cancels a pending show and hides after hide_delay. Touch long
/// press shows immediately and release hides after the grace period. The content receives the popup anchor. Repeated
/// enters update the existing popup instead of stacking another layer.
/// @param content Hover target and popup anchor.
/// @param card Card content factory. Must not be empty.
/// @param style Delays, long press, and popup options.
/// @return A View carrying the hover and long-press bindings.
/// @throws std::invalid_argument If the delays are invalid or card is empty.
View HoverCard(View content, std::function<View(PopupContext)> card, HoverCardStyle style = {});

} // namespace huxerui::interaction
