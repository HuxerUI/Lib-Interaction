#pragma once

#include <huxerui/geometry.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/scroll.h>
#include <huxerui/view.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <utility>

namespace huxerui::interaction {
namespace detail {
struct ScrollSpyControllerState;
} // namespace detail

struct ScrollSpyStyle;

/// Copyable handle exposing the active visible section and programmatic section navigation for ScrollSpyArea.
/// Default-constructed handles are unbound and safe outside an Application runtime; bind one with
/// UseScrollSpyController().
class ScrollSpyController {
public:
  ScrollSpyController() noexcept = default;

  [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(state_); }

  /// Currently active section index (subscribes active composition).
  [[nodiscard]] std::size_t ActiveSection() const;

  /// Number of mounted sections currently registered in the enclosing ScrollSpyArea (subscribes active composition).
  [[nodiscard]] std::size_t SectionCount() const;

  /// Scrolls the connected ScrollController so the leading edge of section `index` aligns with the viewport start.
  /// Returns true when a connected ScrollController accepted the request.
  bool ScrollToSection(std::size_t index) const;

  bool operator==(const ScrollSpyController&) const = default;

private:
  explicit ScrollSpyController(std::shared_ptr<detail::ScrollSpyControllerState> state) : state_(std::move(state)) {}

  std::shared_ptr<detail::ScrollSpyControllerState> state_;

  friend ScrollSpyController UseScrollSpyController(std::size_t initial_section);
  friend View ScrollSpyArea(View content, ScrollSpyStyle style, std::function<void(std::size_t)> on_active_changed);
};

/// Remembers a ScrollSpyController in the current composition scope.
ScrollSpyController UseScrollSpyController(std::size_t initial_section = 0);

/// Configuration for ScrollSpyArea.
struct ScrollSpyStyle {
  /// Optional ScrollController attached to the tracked ScrollView. Required for bottom-of-scroll activation and
  /// programmatic ScrollToSection(). Left empty by default so default-constructed styles compare equal cleanly.
  std::optional<ScrollController> scroll_controller{};
  /// Scroll axis along which section leading edges are compared.
  Axis axis = Axis::Vertical;
  /// Distance in logical pixels inside the leading viewport edge where a section becomes active. Must be finite and
  /// non-negative.
  float activation_offset = 16.0F;
  /// Optional external controller bound to this ScrollSpyArea.
  ScrollSpyController controller{};

  bool operator==(const ScrollSpyStyle&) const = default;
};

/// Wraps a scrollable viewport (typically `ScrollView(...) | ScrollSpyArea(...)`) and tracks which descendant
/// `ScrollSpySection` is currently active near the leading edge of the viewport.
/// @throws std::invalid_argument If `style.activation_offset` is non-finite or negative.
View ScrollSpyArea(View content, ScrollSpyStyle style = {}, std::function<void(std::size_t)> on_active_changed = {});

/// Pipeable adaptor for ScrollSpyArea.
[[nodiscard]] inline ViewAdaptor ScrollSpyArea(ScrollSpyStyle style = {},
                                               std::function<void(std::size_t)> on_active_changed = {}) {
  return ViewAdaptor([style, on_active_changed = std::move(on_active_changed)](View content) {
    return ScrollSpyArea(std::move(content), style, on_active_changed);
  });
}

/// Registers a child view at `index` inside the enclosing ScrollSpyArea so its live presentation geometry drives
/// active-section tracking and `ScrollSpyController::ScrollToSection(index)`.
View ScrollSpySection(View content, std::size_t index);

/// Pipeable adaptor for ScrollSpySection.
[[nodiscard]] inline ViewAdaptor ScrollSpySection(std::size_t index) {
  return ViewAdaptor([index](View content) { return ScrollSpySection(std::move(content), index); });
}

} // namespace huxerui::interaction
