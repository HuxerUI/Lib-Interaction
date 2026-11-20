#pragma once

#include <huxerui/animation.h>
#include <huxerui/color.h>
#include <huxerui/event.h>
#include <huxerui/geometry.h>
#include <huxerui/view.h>

#include <functional>
#include <memory>
#include <optional>
#include <utility>

namespace huxerui::interaction {
namespace detail {
struct CompareSliderControllerState;
} // namespace detail

struct CompareSliderSnapshot;
struct CompareSliderStyle;

/// Copyable handle for reading or programmatically driving a mounted CompareSlider split ratio.
/// Default-constructed handles are unbound and safe outside an Application runtime; bind one with
/// UseCompareSliderController().
class CompareSliderController {
public:
  CompareSliderController() noexcept = default;

  [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(state_); }

  /// Current split progress in `[0, 1]` (subscribes active composition). Returns `0.5F` when unbound.
  [[nodiscard]] float Progress() const;

  /// True while the user is actively dragging the split handle (subscribes active composition).
  [[nodiscard]] bool IsDragging() const;

  /// Programmatically moves the split progress toward `progress` (clamped to `[min_progress, max_progress]`).
  /// Animates using `CompareSliderStyle::motion` when `animated` is true, or snaps immediately when false.
  void SetProgress(float progress, bool animated = true) const;

  /// Restores the split progress to `CompareSliderStyle::initial_progress`.
  void Reset(bool animated = true) const;

  bool operator==(const CompareSliderController&) const = default;

private:
  explicit CompareSliderController(std::shared_ptr<detail::CompareSliderControllerState> state)
      : state_(std::move(state)) {}

  std::shared_ptr<detail::CompareSliderControllerState> state_;

  friend CompareSliderController UseCompareSliderController(float initial_progress);
  friend View CompareSlider(View before, View after, std::function<View(CompareSliderSnapshot)> handle_builder,
                            CompareSliderStyle style, std::function<void(float progress)> on_change);
  friend View CompareSlider(View before, View after, CompareSliderStyle style,
                            std::function<void(float progress)> on_change);
};

/// Remembers a CompareSliderController in the current composition scope.
/// @throws std::invalid_argument If `initial_progress` is non-finite or outside `[0, 1]`.
CompareSliderController UseCompareSliderController(float initial_progress = 0.5F);

/// Live split-curtain state passed to headless CompareSlider handle builders.
struct CompareSliderSnapshot {
  /// Normalized split progress in `[0, 1]`.
  float progress = 0.5F;
  /// True while a pointer drag sequence is active.
  bool dragging = false;
  /// True while a pointer hovers an active hit region of the slider.
  bool hovered = false;
  /// True while the slider holds keyboard focus.
  bool focused = false;
  /// Axis along which `progress` splits the `before` and `after` layers.
  Axis axis = Axis::Horizontal;

  bool operator==(const CompareSliderSnapshot&) const = default;
};

/// Split orientation, bounds, input policy, visual styling, and motion configuration for CompareSlider.
struct CompareSliderStyle {
  /// Axis along which the divider travels (`Horizontal` splits left/right; `Vertical` splits top/bottom).
  Axis axis = Axis::Horizontal;
  /// Initial split ratio applied on mount and by Reset(). Must be finite and in `[min_progress, max_progress]`.
  float initial_progress = 0.5F;
  /// Lower normalized split limit in `[0, 1]`. Must be finite and `<= max_progress`.
  float min_progress = 0.0F;
  /// Upper normalized split limit in `[0, 1]`. Must be finite and `>= min_progress`.
  float max_progress = 1.0F;
  /// When true, dragging anywhere within the slider bounds moves the split; when false, only hits within
  /// `handle_hit_extent` centered on the divider capture pointer drags.
  bool drag_anywhere = true;
  /// Interactive hit-band width centered on the divider when `drag_anywhere == false`, in logical pixels.
  /// Must be finite and positive.
  float handle_hit_extent = 32.0F;
  /// Keyboard arrow adjustment step in normalized units. Must be finite and in `(0, 1]`.
  float keyboard_step = 0.05F;
  /// When true, double-clicking the interactive region animates the split back to `initial_progress`.
  bool double_click_resets = true;
  /// Whether the stage participates in keyboard focus navigation and arrow/Home/End/PageUp/PageDown keys.
  bool keyboard_focusable = true;
  /// Width (or height for Vertical) of the separator line along `axis` in logical pixels. Must be finite and >= 0.
  float divider_thickness = 2.0F;
  /// Color of the separator line drawn between `before` and `after`.
  Color divider_color = Color::Rgb(255, 255, 255, 0.92F);
  /// Diameter of the built-in circular drag knob in logical pixels (preset overload). Must be finite and >= 0.
  float handle_size = 32.0F;
  /// Background fill color of the built-in circular knob (preset overload).
  Color handle_color = Color::Rgb(255, 255, 255, 0.96F);
  /// Chevron accent and hover/focus halo tint for the built-in knob (preset overload).
  Color handle_accent_color = Color::Rgb(31, 111, 235, 1.0F);
  /// Animation specification used by programmatic SetProgress(..., animated=true) and Reset().
  AnimationSpec motion = SpringSpec{.stiffness = 380.0F, .damping_ratio = 0.85F};
  /// Optional external controller bound to this CompareSlider.
  CompareSliderController controller{};

  bool operator==(const CompareSliderStyle&) const = default;
};

/// Overlays `before` (clipped to `[0, progress]`) above `after` (full stage), delegating divider handle markup to
/// `handle_builder(CompareSliderSnapshot)` while handling pointer scrubbing, keyboard steps, and Slider semantics.
/// @throws std::invalid_argument If `handle_builder` is empty or `style` contains invalid bounds or dimensions.
View CompareSlider(View before, View after, std::function<View(CompareSliderSnapshot)> handle_builder,
                   CompareSliderStyle style = {}, std::function<void(float progress)> on_change = {});

/// Overlays `before` (clipped to `[0, progress]`) above `after` with a built-in painted divider line and circular
/// chevron knob.
/// @throws std::invalid_argument If `style` contains invalid bounds or dimensions.
View CompareSlider(View before, View after, CompareSliderStyle style = {},
                   std::function<void(float progress)> on_change = {});

} // namespace huxerui::interaction
