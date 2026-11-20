#pragma once

#include <huxerui/animation.h>
#include <huxerui/geometry.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/interaction/scroll/scrollspy.h>
#include <huxerui/interaction/visual/appear.h>
#include <huxerui/layout.h>
#include <huxerui/scroll.h>
#include <huxerui/view.h>

#include <functional>
#include <memory>
#include <optional>
#include <utility>

namespace huxerui::interaction {
namespace detail {
struct ScrollToTopControllerState;
} // namespace detail

struct ScrollToTopContext;
struct ScrollToTopStyle;

/// Copyable handle exposing live scroll-to-top visibility, progress, and programmatic scroll-to-top actions.
/// Default-constructed handles are unbound and safe outside an Application runtime; bind one with
/// UseScrollToTopController().
class ScrollToTopController {
public:
  ScrollToTopController() noexcept = default;

  [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(state_); }

  /// True while the floating trigger threshold and optional scroll-up conditions are satisfied
  /// (subscribes active composition).
  [[nodiscard]] bool IsVisible() const;

  /// True while a programmatic smooth ScrollToTop() animation is running (subscribes active composition).
  [[nodiscard]] bool IsScrollingToTop() const;

  /// Current scroll offset along `axis` in logical pixels (subscribes active composition).
  [[nodiscard]] float Offset() const;

  /// Normalized `[0, 1]` scroll progress relative to maximum offset (subscribes active composition).
  [[nodiscard]] float Progress() const;

  /// Scrolls the connected ScrollController to `target_offset` using `behavior` (or `style.jump_behavior` if unset).
  /// Returns true when a connected ScrollController on `axis` accepted the request.
  bool ScrollToTop(std::optional<ScrollJumpBehavior> behavior = std::nullopt) const;

  /// Cancels an active smooth ScrollToTop() animation, if any.
  void Cancel() const;

  bool operator==(const ScrollToTopController&) const = default;

private:
  explicit ScrollToTopController(std::shared_ptr<detail::ScrollToTopControllerState> state)
      : state_(std::move(state)) {}

  std::shared_ptr<detail::ScrollToTopControllerState> state_;

  friend ScrollToTopController UseScrollToTopController();
  friend View ScrollToTop(View scrollable_content, std::function<View(ScrollToTopContext)> trigger_builder,
                          ScrollToTopStyle style, std::function<void()> on_triggered);
  friend View ScrollToTopTrigger(View trigger, ScrollToTopController controller,
                                 std::optional<ScrollJumpBehavior> behavior, std::function<void()> on_triggered);
};

/// Remembers a ScrollToTopController in the current composition scope.
ScrollToTopController UseScrollToTopController();

/// Live scroll telemetry and action helpers passed to headless ScrollToTop trigger builders.
struct ScrollToTopContext {
  /// True when the floating trigger is currently eligible to appear.
  bool visible = false;
  /// True while a programmatic smooth scroll-to-top animation is running.
  bool scrolling_to_top = false;
  /// Current scroll offset along `axis` in logical pixels.
  float offset = 0.0F;
  /// Normalized `[0, 1]` scroll progress relative to maximum scroll extent.
  float progress = 0.0F;
  /// Bound controller for programmatic actions and observation.
  ScrollToTopController controller{};

  /// Programmatically scrolls the bound container back to `target_offset`.
  bool ScrollToTop(std::optional<ScrollJumpBehavior> behavior = std::nullopt) const {
    return controller.ScrollToTop(behavior);
  }

  /// Cancels an active smooth ScrollToTop() animation.
  void Cancel() const { controller.Cancel(); }

  bool operator==(const ScrollToTopContext&) const = default;
};

/// Visibility threshold, motion, overlay placement, and controller configuration for ScrollToTop.
struct ScrollToTopStyle {
  /// ScrollController bound to the scrollable container to track and jump. Must be attached to the target
  /// ScrollView so visibility, progress, and ScrollToTop() connect on `axis`.
  std::optional<ScrollController> scroll_controller{};
  /// Scroll axis monitored for offset, threshold, and jump motion.
  Axis axis = Axis::Vertical;
  /// Scroll offset in logical pixels required before the trigger becomes visible. Must be finite and >= 0.
  float threshold = 160.0F;
  /// Destination offset along `axis` reached by ScrollToTop(), in logical pixels. Must be finite and >= 0.
  float target_offset = 0.0F;
  /// When true, reveals the trigger only after scrolling backward past `threshold` by `scroll_up_delta`,
  /// hiding again when scrolling forward or returning within `threshold`.
  bool show_on_scroll_up = false;
  /// Minimum backward travel in logical pixels to reveal (or forward travel to hide) when `show_on_scroll_up`
  /// is enabled. Must be finite and >= 0.
  float scroll_up_delta = 12.0F;
  /// Default transition mode used by ScrollToTop() when no explicit behavior override is passed.
  ScrollJumpBehavior jump_behavior = ScrollJumpBehavior::Smooth;
  /// Animation timing or spring specification when `jump_behavior` resolves to `ScrollJumpBehavior::Smooth`.
  AnimationSpec scroll_motion = TweenSpec{.duration = 0.35, .easing = Easing::EaseInOut};
  /// Appear transition applied to the floating trigger overlay as it mounts into view.
  AppearStyle reveal_motion{};
  /// Horizontal alignment of the floating trigger inside the scrollable viewport.
  HorizontalAlignment horizontal_alignment = HorizontalAlignment::End;
  /// Vertical alignment of the floating trigger inside the scrollable viewport.
  VerticalAlignment vertical_alignment = VerticalAlignment::End;
  /// Edge margins around the floating trigger inside the viewport. All edges must be finite and >= 0.
  EdgeInsets margin = EdgeInsets::All(16.0F);
  /// When true, user wheel, drag, or scrollbar input cancels an active smooth scroll-to-top animation.
  bool interrupt_on_user_scroll = true;
  /// Optional external controller bound to this ScrollToTop container.
  ScrollToTopController controller{};

  bool operator==(const ScrollToTopStyle&) const = default;
};

/// Overlays an animated floating scroll-to-top trigger over `scrollable_content`, delegating trigger markup and
/// interaction wiring to `trigger_builder(ScrollToTopContext)`.
/// @throws std::invalid_argument If `trigger_builder` is empty or `style` contains invalid bounds or motions.
View ScrollToTop(View scrollable_content, std::function<View(ScrollToTopContext)> trigger_builder,
                 ScrollToTopStyle style = {}, std::function<void()> on_triggered = {});

/// Wraps `trigger` as an interactive clickable overlay over `scrollable_content` that jumps to `target_offset`
/// according to `style`.
/// @throws std::invalid_argument If `style` contains invalid bounds or motions.
View ScrollToTop(View scrollable_content, View trigger, ScrollToTopStyle style = {},
                 std::function<void()> on_triggered = {});

/// Pipeable adaptor for headless builder ScrollToTop.
[[nodiscard]] inline ViewAdaptor ScrollToTop(std::function<View(ScrollToTopContext)> trigger_builder,
                                             ScrollToTopStyle style = {},
                                             std::function<void()> on_triggered = {}) {
  return ViewAdaptor([trigger_builder = std::move(trigger_builder), style = std::move(style),
                      on_triggered = std::move(on_triggered)](View scrollable_content) {
    return ScrollToTop(std::move(scrollable_content), trigger_builder, style, on_triggered);
  });
}

/// Pipeable adaptor for wrapped-trigger ScrollToTop.
[[nodiscard]] inline ViewAdaptor ScrollToTop(View trigger, ScrollToTopStyle style = {},
                                             std::function<void()> on_triggered = {}) {
  return ViewAdaptor([trigger = std::move(trigger), style = std::move(style),
                      on_triggered = std::move(on_triggered)](View scrollable_content) {
    return ScrollToTop(std::move(scrollable_content), trigger, style, on_triggered);
  });
}

/// Wraps an inline or external `trigger` view (such as a header button) to activate the enclosing ScrollToTop
/// environment or an explicit `controller`.
View ScrollToTopTrigger(View trigger, ScrollToTopController controller = {},
                        std::optional<ScrollJumpBehavior> behavior = std::nullopt,
                        std::function<void()> on_triggered = {});

/// Pipeable adaptor for ScrollToTopTrigger.
[[nodiscard]] inline ViewAdaptor ScrollToTopTrigger(ScrollToTopController controller = {},
                                                    std::optional<ScrollJumpBehavior> behavior = std::nullopt,
                                                    std::function<void()> on_triggered = {}) {
  return ViewAdaptor([controller = std::move(controller), behavior,
                      on_triggered = std::move(on_triggered)](View trigger) {
    return ScrollToTopTrigger(std::move(trigger), controller, behavior, on_triggered);
  });
}

} // namespace huxerui::interaction
