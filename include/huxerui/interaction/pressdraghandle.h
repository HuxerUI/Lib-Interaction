#pragma once

#include <huxerui/event.h>
#include <huxerui/gesture.h>
#include <huxerui/view.h>

#include <chrono>
#include <functional>
#include <optional>

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

/// Requires a hold before a drag is recognized, without moving the content itself.
///
/// This is the building block for reorder grips and press-drag surfaces. It attaches DragGesture with
/// minimum_press_duration and forwards the typed lifecycle to the callbacks; the caller owns any offset or model
/// mutation. The wrapper imposes no intrinsic frame.
/// @param content Handle content.
/// @param style Hold duration, axis, and cursor.
/// @param callbacks Drag lifecycle callbacks.
/// @return A View carrying the press-drag recognizer.
/// @throws std::invalid_argument If the hold duration is not positive.
View PressDragHandle(View content, PressDragHandleStyle style = {}, PressDragHandleCallbacks callbacks = {});

} // namespace huxerui::interaction
