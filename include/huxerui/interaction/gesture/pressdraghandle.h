#pragma once

#include <huxerui/event.h>
#include <huxerui/gesture.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <chrono>
#include <functional>
#include <optional>
#include <utility>

namespace huxerui::interaction {

/// Configuration for PressDragHandle.
struct PressDragHandleStyle {
  /// Hold duration before drag recognition. Must be finite and positive.
  std::chrono::duration<double> press_duration{0.3};
  /// Optional axis constraint. Empty allows two-dimensional movement.
  std::optional<Axis> axis = std::nullopt;
  /// Cursor shown while the handle is idle.
  PointerCursorKind cursor = PointerCursorKind::Grab;

  bool operator==(const PressDragHandleStyle&) const = default;
};

/// Callbacks forwarded from a press-then-drag handle.
struct PressDragHandleCallbacks {
  std::function<void(const DragEvent&)> on_started;
  std::function<void(const DragEvent&)> on_changed;
  std::function<void(const DragEvent&)> on_ended;
  std::function<void(const DragEvent&)> on_canceled;
};

/// Visual/cursor-only PressDragHandle overload.
View PressDragHandle(View content, PressDragHandleStyle style = {});

/// Requires a hold before a drag is recognized, forwarding lifecycle events to `callbacks` without moving content itself.
/// @throws std::invalid_argument If `press_duration` is non-finite or not positive.
View PressDragHandle(View content, PressDragHandleCallbacks callbacks, PressDragHandleStyle style = {});

/// Pipeable adaptor for PressDragHandle.
[[nodiscard]] inline ViewAdaptor PressDragHandle(PressDragHandleCallbacks callbacks,
                                                 PressDragHandleStyle style = {}) {
  return ViewAdaptor([callbacks = std::move(callbacks), style](View content) {
    return PressDragHandle(std::move(content), callbacks, style);
  });
}

} // namespace huxerui::interaction
