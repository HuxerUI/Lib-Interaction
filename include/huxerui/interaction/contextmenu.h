#pragma once

#include <huxerui/event.h>
#include <huxerui/gesture.h>
#include <huxerui/geometry.h>
#include <huxerui/view.h>

#include <chrono>
#include <functional>
#include <optional>

namespace huxerui::interaction {

/// Recognition configuration for ContextMenuArea.
struct ContextMenuAreaStyle {
  /// Whether a touch long press requests the context menu. Desktop secondary clicks always use
  /// ContextMenuRequested and are unaffected by this flag.
  bool long_press = true;
  /// Touch hold duration before the request. An empty value uses the platform GestureSettings duration. Must be
  /// positive when set.
  std::optional<std::chrono::duration<double>> long_press_duration = std::nullopt;
  bool operator==(const ContextMenuAreaStyle&) const = default;
};

/// Adds a uniform context-menu request surface to content.
///
/// Desktop secondary clicks and the Context Menu key arrive through ViewEvents::ContextMenuRequested. Touch input
/// does not produce that event, so when style.long_press is set this wrapper also recognizes a touch LongPressGesture
/// and forwards its window position. The callback receives window-local logical coordinates and should show the
/// application's menu or popup.
///
/// The wrapper does not consume ordinary pointer events, clicks, or gestures. It imposes no intrinsic frame.
/// @param content Content receiving the context-menu request.
/// @param on_request Callback invoked with the window-local request position; an empty function makes the wrapper a
///     no-op observer that still leaves other bindings intact.
/// @param style Recognition configuration.
/// @return A View with the context-menu bindings attached.
/// @throws std::invalid_argument If the long-press duration override is not positive.
/// @code
/// ContextMenuArea(Card(), [menu](Point position) { menu.ShowAt(position, items); });
/// @endcode
View ContextMenuArea(View content, std::function<void(Point)> on_request, ContextMenuAreaStyle style = {});

} // namespace huxerui::interaction
