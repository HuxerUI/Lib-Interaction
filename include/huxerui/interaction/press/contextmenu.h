#pragma once

#include <huxerui/event.h>
#include <huxerui/geometry.h>
#include <huxerui/gesture.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <chrono>
#include <functional>
#include <optional>
#include <utility>

namespace huxerui::interaction {

/// Recognition configuration for ContextMenuArea.
struct ContextMenuAreaStyle {
  /// Whether a touch long press also triggers `on_request`. Desktop secondary clicks and Context Menu keys always route.
  bool long_press = true;
  /// Touch hold duration override before the request. Empty uses platform GestureSettings.
  std::optional<std::chrono::duration<double>> long_press_duration = std::nullopt;
  /// Cursor shown over the content.
  PointerCursorKind cursor = PointerCursorKind::Default;
  /// Ensures otherwise inert views participate in pointer hit testing so secondary clicks reach them.
  bool hit_test_inert = true;

  bool operator==(const ContextMenuAreaStyle&) const = default;
};

/// Configuration for SecondaryClickable (desktop/keyboard-only convenience over ContextMenuArea).
struct SecondaryClickableStyle {
  /// Cursor shown over the content.
  PointerCursorKind cursor = PointerCursorKind::Default;

  bool operator==(const SecondaryClickableStyle&) const = default;
};

/// Routes desktop secondary-click, Context Menu key / Shift+F10, and optional touch long-press requests to `on_request`
/// with window-local logical coordinates, without creating a popup itself or consuming primary clicks.
/// @throws std::invalid_argument If `long_press_duration` is non-finite or non-positive.
View ContextMenuArea(View content, std::function<void(Point)> on_request, ContextMenuAreaStyle style = {});

/// Pipeable adaptor for ContextMenuArea.
[[nodiscard]] inline ViewAdaptor ContextMenuArea(std::function<void(Point)> on_request,
                                                 ContextMenuAreaStyle style = {}) {
  return ViewAdaptor([on_request = std::move(on_request), style](View content) {
    return ContextMenuArea(std::move(content), on_request, style);
  });
}

/// Routes secondary-click and Context Menu key requests without synthesizing touch long press.
inline View SecondaryClickable(View content, std::function<void(Point)> on_secondary,
                               SecondaryClickableStyle style = {}) {
  return ContextMenuArea(std::move(content), std::move(on_secondary),
                         ContextMenuAreaStyle{.long_press = false, .cursor = style.cursor});
}

/// Pipeable adaptor for SecondaryClickable.
[[nodiscard]] inline ViewAdaptor SecondaryClickable(std::function<void(Point)> on_secondary,
                                                    SecondaryClickableStyle style = {}) {
  return ViewAdaptor([on_secondary = std::move(on_secondary), style](View content) {
    return SecondaryClickable(std::move(content), on_secondary, style);
  });
}

} // namespace huxerui::interaction
