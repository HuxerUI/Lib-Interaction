#pragma once

#include <huxerui/event.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <chrono>
#include <optional>
#include <utility>

namespace huxerui::interaction {

/// Configuration for ReorderHandle.
struct ReorderHandleStyle {
  /// Optional hold duration before drag initiation. Empty starts dragging immediately on pointer drag.
  std::optional<std::chrono::duration<double>> minimum_press_duration = std::nullopt;
  /// Cursor shown while the handle is idle; switches to Grabbing while dragging its owning slot.
  PointerCursorKind cursor = PointerCursorKind::Grab;

  bool operator==(const ReorderHandleStyle&) const = default;
};

/// Marks `content` as a drag grip for its enclosing Reorderable or ReorderableGrid slot.
///
/// Combine with `ReorderableStyle::handle_only = true` (or `ReorderableGridStyle::handle_only = true`) so interactive
/// controls inside the row or cell do not compete with whole-slot dragging.
/// @throws std::invalid_argument If `minimum_press_duration` is non-finite or not positive.
View ReorderHandle(View content, ReorderHandleStyle style = {});

/// Pipeable adaptor for ReorderHandle.
[[nodiscard]] inline ViewAdaptor ReorderHandle(ReorderHandleStyle style = {}) {
  return ViewAdaptor([style](View content) { return ReorderHandle(std::move(content), style); });
}

} // namespace huxerui::interaction
