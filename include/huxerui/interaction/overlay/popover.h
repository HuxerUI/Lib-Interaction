#pragma once

#include <huxerui/geometry.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/presentation.h>
#include <huxerui/view.h>

#include <functional>
#include <memory>
#include <utility>

namespace huxerui::interaction {
namespace detail {
struct PopoverControllerState;
} // namespace detail

struct PopoverStyle;
struct PopoverTriggerContext;

/// Copyable handle for observing or programmatically opening/closing a mounted Popover.
/// Default-constructed handles are unbound and safe outside an Application runtime; bind one with
/// UsePopoverController().
class PopoverController {
public:
  PopoverController() noexcept = default;

  [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(state_); }

  /// Reports whether the bound Popover is currently open (subscribes active composition).
  [[nodiscard]] bool IsOpen() const;

  /// Opens the bound Popover when mounted.
  void Open() const;

  /// Closes the bound Popover when mounted.
  void Close() const;

  /// Toggles between open and closed states.
  void Toggle() const;

  bool operator==(const PopoverController&) const = default;

private:
  explicit PopoverController(std::shared_ptr<detail::PopoverControllerState> state) : state_(std::move(state)) {}

  std::shared_ptr<detail::PopoverControllerState> state_;

  friend PopoverController UsePopoverController();
  friend View Popover(std::function<View(PopoverTriggerContext)> trigger, std::function<View(PopupContext)> content,
                   PopoverStyle style, std::function<void(bool)> on_open_changed);
};

/// Remembers a PopoverController in the current composition scope.
PopoverController UsePopoverController();

/// Configuration for Popover placement, focus confinement, and dismissal policy.
struct PopoverStyle {
  /// Preferred side and cross-axis alignment relative to the anchor.
  AnchorPlacement placement{};
  /// Distance from the anchor in logical pixels. Must be finite and non-negative.
  float gap = 6.0F;
  /// Minimum distance from viewport safe-area edges in logical pixels. Must be finite and non-negative.
  float viewport_margin = 8.0F;
  /// Additional translation from the resolved anchored position. Must be finite.
  Point offset{};
  /// Whether a pointer press outside the popup dismisses the popover.
  bool dismiss_on_outside_press = true;
  /// Whether Escape or platform Back dismisses the popover.
  bool dismiss_on_cancel = true;
  /// Whether keyboard focus is confined to the popover while open.
  bool trap_focus = true;
  /// Keeps keyboard focus on the mounted anchor when pointer input targets popup content without its own focus target.
  bool retain_anchor_focus = false;
  /// Optional external controller connected to this Popover instance.
  PopoverController controller{};

  bool operator==(const PopoverStyle&) const = default;
};

/// State and controls supplied to headless Popover trigger builders.
struct PopoverTriggerContext {
  /// True when the popover is currently open.
  bool open = false;
  /// Opens the popover.
  std::function<void()> show;
  /// Closes the popover.
  std::function<void()> hide;
  /// Toggles the popover open or closed.
  std::function<void()> toggle;
};

/// Headless click-triggered Popover delegating trigger markup and click wiring to `trigger(PopoverTriggerContext)`.
/// @throws std::invalid_argument If `trigger`, `content`, or geometry fields in `style` are invalid.
View Popover(std::function<View(PopoverTriggerContext)> trigger, std::function<View(PopupContext)> content,
             PopoverStyle style = {}, std::function<void(bool)> on_open_changed = {});

/// Wraps `trigger` so clicking it toggles an anchored popover displaying `content`.
/// @throws std::invalid_argument If `content` or geometry fields in `style` are invalid.
View Popover(View trigger, std::function<View(PopupContext)> content, PopoverStyle style = {},
             std::function<void(bool)> on_open_changed = {});

/// Pipeable adaptor for wrapped Popover.
[[nodiscard]] inline ViewAdaptor Popover(std::function<View(PopupContext)> content, PopoverStyle style = {},
                                         std::function<void(bool)> on_open_changed = {}) {
  return ViewAdaptor(
      [content = std::move(content), style, on_open_changed = std::move(on_open_changed)](View trigger) {
        return Popover(std::move(trigger), content, style, on_open_changed);
      });
}

} // namespace huxerui::interaction
