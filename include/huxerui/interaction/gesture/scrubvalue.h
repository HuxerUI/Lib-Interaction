#pragma once

#include <huxerui/event.h>
#include <huxerui/geometry.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <utility>

namespace huxerui::interaction {
namespace detail {
struct ScrubValueControllerState;
} // namespace detail

struct ScrubValueSnapshot;
struct ScrubValueStyle;

/// Active pointer/keyboard modifier sensitivity tier during a ScrubValue gesture.
enum class ScrubPrecision {
  Normal,
  Fine,
  Coarse,
};

/// Live value, active drag delta, modifier precision, and interaction state passed to ScrubValue builders.
struct ScrubValueSnapshot {
  /// Current clamped and quantized numeric value.
  double value = 0.0;
  /// Net change accumulated from the start of the active scrub gesture (`0.0` when idle).
  double drag_delta = 0.0;
  /// True while a pointer drag scrub sequence is active past `drag_slop`.
  bool scrubbing = false;
  /// True while a pointer hovers the scrub surface.
  bool hovered = false;
  /// True while the scrub surface holds keyboard focus.
  bool focused = false;
  /// Active sensitivity multiplier tier (`Shift` = Fine, `Alt` = Coarse).
  ScrubPrecision precision = ScrubPrecision::Normal;

  bool operator==(const ScrubValueSnapshot&) const = default;
};

/// Copyable handle exposing live value, scrub state, and programmatic value control.
/// Default-constructed handles are unbound and safe outside an Application runtime; bind one with
/// UseScrubValueController().
class ScrubValueController {
public:
  ScrubValueController() noexcept = default;

  [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(state_); }

  /// Current numeric value (subscribes active composition). Returns `0.0` when unbound.
  [[nodiscard]] double Value() const;

  /// True while the user is actively dragging past `drag_slop` (subscribes active composition).
  [[nodiscard]] bool IsScrubbing() const;

  /// Current precision tier (`Normal`, `Fine`, or `Coarse`; subscribes active composition).
  [[nodiscard]] ScrubPrecision Precision() const;

  /// Clamps, quantizes (if `step` is set), and commits `value`, firing `on_change` / `on_commit` when changed.
  void SetValue(double value) const;

  /// Adds `delta` to the current value using the bound clamping and step rules.
  void Nudge(double delta) const;

  /// Resets the bound value to `default_value` (or `initial_value` when `default_value` is unset).
  void Reset() const;

  bool operator==(const ScrubValueController&) const = default;

private:
  explicit ScrubValueController(std::shared_ptr<detail::ScrubValueControllerState> state)
      : state_(std::move(state)) {}

  std::shared_ptr<detail::ScrubValueControllerState> state_;

  friend ScrubValueController UseScrubValueController(double initial_value);
  friend View ScrubValue(std::function<View(ScrubValueSnapshot)> builder, ScrubValueStyle style,
                         std::function<void(double value)> on_change,
                         std::function<void(double value)> on_commit);
  friend View ScrubValue(View content, ScrubValueStyle style, std::function<void(double value)> on_change,
                         std::function<void(double value)> on_commit);
};

/// Remembers a ScrubValueController in the current composition scope.
/// @throws std::invalid_argument If `initial_value` is non-finite.
ScrubValueController UseScrubValueController(double initial_value = 0.0);

/// Bounds, sensitivity multipliers, step quantization, axis, and controller configuration for ScrubValue.
struct ScrubValueStyle {
  /// Initial value used on mount (and fallback target for Reset() when `default_value` is empty). Must be finite
  /// and within `[min_value, max_value]`.
  double initial_value = 0.0;
  /// Optional reset target used on double-click or Reset(). When set, must be finite and in `[min_value, max_value]`.
  std::optional<double> default_value{};
  /// Inclusive lower bound (may be `-infinity` for unbounded lower range). Must not be NaN or `+infinity`.
  double min_value = -std::numeric_limits<double>::infinity();
  /// Inclusive upper bound (may be `+infinity` for unbounded upper range). Must not be NaN or `-infinity`, and
  /// must satisfy `min_value <= max_value`.
  double max_value = std::numeric_limits<double>::infinity();
  /// Value units applied per logical pixel along the active axis. Must be finite and positive.
  double sensitivity = 1.0;
  /// Sensitivity multiplier when `Shift` is held (`Fine` precision). Must be finite and positive.
  double fine_multiplier = 0.1;
  /// Sensitivity multiplier when `Alt` is held (`Coarse` precision). Must be finite and positive.
  double coarse_multiplier = 10.0;
  /// Optional quantization step increment anchored at `min_value` (if finite) or `0.0`. Must be finite and positive.
  std::optional<double> step{};
  /// Base keyboard increment used when `step` is unset. Must be finite and positive.
  double keyboard_step = 1.0;
  /// Scrub axis (`Horizontal` increases rightward; `Vertical` increases upward unless `invert_axis` is true).
  Axis axis = Axis::Horizontal;
  /// Reverses the increasing pointer direction along `axis`.
  bool invert_axis = false;
  /// Minimum pointer movement in logical pixels before a press enters scrubbing. Must be finite and >= 0.
  float drag_slop = 2.0F;
  /// When true, double-clicking without dragging resets to `default_value.value_or(initial_value)`.
  bool double_click_resets = true;
  /// Whether the scrub surface participates in keyboard focus and arrow/Home/End/PageUp/PageDown keys.
  bool keyboard_focusable = true;
  /// Cursor displayed over the scrub surface (`Default` auto-selects `ResizeHorizontal` / `ResizeVertical`).
  PointerCursorKind cursor = PointerCursorKind::Default;
  /// Optional external controller bound to this ScrubValue instance.
  ScrubValueController controller{};

  bool operator==(const ScrubValueStyle&) const = default;
};

/// Headless numeric drag scrubber delegating presentation to `builder(ScrubValueSnapshot)`.
/// @throws std::invalid_argument If `builder` is empty or `style` contains invalid bounds or multipliers.
View ScrubValue(std::function<View(ScrubValueSnapshot)> builder, ScrubValueStyle style = {},
                std::function<void(double value)> on_change = {},
                std::function<void(double value)> on_commit = {});

/// Wraps `content` as an interactive numeric drag scrubber controlled by `style`.
/// @throws std::invalid_argument If `style` contains invalid bounds or multipliers.
View ScrubValue(View content, ScrubValueStyle style = {}, std::function<void(double value)> on_change = {},
                std::function<void(double value)> on_commit = {});

/// Pipeable adaptor for wrapped ScrubValue.
[[nodiscard]] inline ViewAdaptor ScrubValue(ScrubValueStyle style = {},
                                            std::function<void(double value)> on_change = {},
                                            std::function<void(double value)> on_commit = {}) {
  return ViewAdaptor(
      [style = std::move(style), on_change = std::move(on_change), on_commit = std::move(on_commit)](View content) {
        return ScrubValue(std::move(content), style, on_change, on_commit);
      });
}

} // namespace huxerui::interaction
