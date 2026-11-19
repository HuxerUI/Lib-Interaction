#pragma once

#include <huxerui/event.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <chrono>
#include <functional>
#include <string>
#include <utility>

namespace huxerui::interaction {

/// Configuration for click-to-copy Copyable wrappers and builders.
struct CopyableStyle {
  /// Duration that `copied` feedback remains active after a successful write. Must be finite and positive.
  std::chrono::duration<double> feedback_duration{1.5};
  /// Pointer cursor while hovering the copy trigger.
  PointerCursorKind cursor = PointerCursorKind::Hand;
  /// When true, applies `Enabled(false)` whenever the runtime clipboard service is unavailable.
  bool disable_when_unavailable = false;

  bool operator==(const CopyableStyle&) const = default;
};

/// Headless click-to-copy overload delegating presentation to `builder(bool copied)`.
/// @throws std::invalid_argument If `builder` is empty or `style.feedback_duration` is non-positive/non-finite.
View Copyable(std::function<View(bool copied)> builder, std::string text,
              std::function<void(bool succeeded)> on_copied = {}, CopyableStyle style = {});

/// Headless click-to-copy overload without an explicit completion callback.
View Copyable(std::function<View(bool copied)> builder, std::string text, CopyableStyle style);

/// Wraps `content` so clicking copies `text` to the platform clipboard.
/// @throws std::invalid_argument If `style.feedback_duration` is non-positive/non-finite.
View Copyable(View content, std::string text, std::function<void(bool succeeded)> on_copied = {},
              CopyableStyle style = {});

/// Style-only wrapped Copyable overload without an explicit completion callback.
View Copyable(View content, std::string text, CopyableStyle style);

/// Pipeable adaptor for Copyable.
[[nodiscard]] inline ViewAdaptor Copyable(std::string text, std::function<void(bool succeeded)> on_copied = {},
                                          CopyableStyle style = {}) {
  return ViewAdaptor([text = std::move(text), on_copied = std::move(on_copied), style](View content) {
    return Copyable(std::move(content), text, on_copied, style);
  });
}

/// Pipeable style-only adaptor for Copyable.
[[nodiscard]] inline ViewAdaptor Copyable(std::string text, CopyableStyle style) {
  return ViewAdaptor(
      [text = std::move(text), style](View content) { return Copyable(std::move(content), text, style); });
}

} // namespace huxerui::interaction
