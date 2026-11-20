#pragma once

#include <huxerui/animation.h>
#include <huxerui/geometry.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/interaction/scroll/scrollspy.h>
#include <huxerui/scroll.h>
#include <huxerui/view.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace huxerui::interaction {
namespace detail {
struct StickyHeaderControllerState;
} // namespace detail

struct StickyHeaderStyle;
struct StickyHeaderGroup;

/// Live sticky pinning and push-out progress state passed to header builders.
struct StickyHeaderSnapshot {
  /// Index of the section header represented by this snapshot.
  std::size_t index = 0;
  /// True when rendered as the pinned viewport overlay; false for the in-flow section header.
  bool pinned = false;
  /// Normalized `[0, 1]` push-out progress as the next section boundary approaches the pinned header.
  float push_progress = 0.0F;

  bool operator==(const StickyHeaderSnapshot&) const = default;
};

/// Copyable handle exposing the active pinned section, push progress, and programmatic section jumping.
/// Default-constructed handles are unbound and safe outside an Application runtime; bind one with
/// UseStickyHeaderController().
class StickyHeaderController {
public:
  StickyHeaderController() noexcept = default;

  [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(state_); }

  /// Currently pinned section index, or empty when no section is currently pinned (subscribes active composition).
  [[nodiscard]] std::optional<std::size_t> PinnedSection() const;

  /// True while a section header is currently pinned in the overlay slot (subscribes active composition).
  [[nodiscard]] bool IsPinned() const;

  /// Current `[0, 1]` push-out progress of the active pinned header (subscribes active composition).
  [[nodiscard]] float PushProgress() const;

  /// Number of mounted StickyHeaderSection entries registered in the area (subscribes active composition).
  [[nodiscard]] std::size_t SectionCount() const;

  /// Scrolls the connected ScrollController so the leading edge of section `index` aligns with `sticky_offset`.
  /// When `behavior` is unset, follows `StickyHeaderStyle::jump_behavior`.
  /// Returns true when a connected ScrollController accepted the request.
  bool ScrollToSection(std::size_t index, std::optional<ScrollJumpBehavior> behavior = std::nullopt) const;

  bool operator==(const StickyHeaderController&) const = default;

private:
  explicit StickyHeaderController(std::shared_ptr<detail::StickyHeaderControllerState> state)
      : state_(std::move(state)) {}

  std::shared_ptr<detail::StickyHeaderControllerState> state_;

  friend StickyHeaderController UseStickyHeaderController();
  friend View StickyHeaderArea(View scrollable_content,
                               std::function<View(StickyHeaderSnapshot)> pinned_header_builder, StickyHeaderStyle style,
                               std::function<void(std::optional<std::size_t>)> on_pinned_changed);
  friend View StickyHeaderList(std::vector<StickyHeaderGroup> groups, StickyHeaderStyle style,
                               std::function<void(std::optional<std::size_t>)> on_pinned_changed);
};

/// Remembers a StickyHeaderController in the current composition scope.
StickyHeaderController UseStickyHeaderController();

/// Axis, offset, jump-motion, and controller configuration for StickyHeaderArea and StickyHeaderList.
struct StickyHeaderStyle {
  /// Optional ScrollController bound to the scrollable content. Enables zero-lag pinned updates, rest detection,
  /// and programmatic ScrollToSection().
  std::optional<ScrollController> scroll_controller{};
  /// Scroll and pinning axis (top edge for Vertical, leading edge for Horizontal).
  Axis axis = Axis::Vertical;
  /// Inset from the leading viewport edge where active headers pin, in logical pixels. Must be finite and >= 0.
  float sticky_offset = 0.0F;
  /// Whether a leading section positioned flush at `sticky_offset` at rest (`scroll_offset == 0`) begins pinned.
  bool pin_at_rest = false;
  /// Controls whether ScrollToSection() animates smoothly or jumps immediately by default.
  ScrollJumpBehavior jump_behavior = ScrollJumpBehavior::Smooth;
  /// Animation timing specification used when `jump_behavior` resolves to `ScrollJumpBehavior::Smooth`.
  AnimationSpec scroll_motion = TweenSpec{.duration = 0.28, .easing = Easing::EaseInOut};
  /// Inter-group spacing applied by StickyHeaderList along `axis`, in logical pixels. Must be finite and >= 0.
  float section_spacing = 0.0F;
  /// Optional external controller bound to this StickyHeaderArea or StickyHeaderList.
  StickyHeaderController controller{};

  bool operator==(const StickyHeaderStyle&) const = default;
};

/// Single grouped section rendered by StickyHeaderList, pairing a snapshot-driven header with body content.
struct StickyHeaderGroup {
  /// Builder receiving StickyHeaderSnapshot (`pinned == false` in flow, `pinned == true` when sticky). Must not be empty.
  std::function<View(StickyHeaderSnapshot)> header;
  /// Scrollable body content belonging to this section.
  View body;
};

/// Wraps a scrollable viewport (typically `ScrollView(...) | StickyHeaderArea(...)`) and pins the active
/// `StickyHeaderSection`'s overlay header while scrolling, pushing it out smoothly as the next section arrives.
/// @throws std::invalid_argument If `pinned_header_builder` is empty or `style` contains invalid bounds.
View StickyHeaderArea(View scrollable_content, std::function<View(StickyHeaderSnapshot)> pinned_header_builder,
                      StickyHeaderStyle style = {},
                      std::function<void(std::optional<std::size_t>)> on_pinned_changed = {});

/// Pipeable adaptor for StickyHeaderArea.
[[nodiscard]] inline ViewAdaptor StickyHeaderArea(
    std::function<View(StickyHeaderSnapshot)> pinned_header_builder, StickyHeaderStyle style = {},
    std::function<void(std::optional<std::size_t>)> on_pinned_changed = {}) {
  return ViewAdaptor([pinned_header_builder = std::move(pinned_header_builder), style = std::move(style),
                      on_pinned_changed = std::move(on_pinned_changed)](View scrollable_content) {
    return StickyHeaderArea(std::move(scrollable_content), pinned_header_builder, style, on_pinned_changed);
  });
}

/// Registers a grouped section view at `index` inside an enclosing StickyHeaderArea so its bounds drive
/// sticky pinning, push-out collision, and StickyHeaderController::ScrollToSection(index).
View StickyHeaderSection(View content, std::size_t index);

/// Pipeable adaptor for StickyHeaderSection.
[[nodiscard]] inline ViewAdaptor StickyHeaderSection(std::size_t index) {
  return ViewAdaptor([index](View content) { return StickyHeaderSection(std::move(content), index); });
}

/// High-level grouped scrollable container that composes sections (`StickyHeaderGroup`) inside a ScrollView
/// paired with StickyHeaderArea, keeping in-flow headers hidden while pinned so body content avoids recomposition.
/// @throws std::invalid_argument If any `StickyHeaderGroup::header` is empty or `style` contains invalid bounds.
View StickyHeaderList(std::vector<StickyHeaderGroup> groups, StickyHeaderStyle style = {},
                      std::function<void(std::optional<std::size_t>)> on_pinned_changed = {});

} // namespace huxerui::interaction
