#pragma once

#include <huxerui/color.h>
#include <huxerui/event.h>
#include <huxerui/geometry.h>
#include <huxerui/view.h>

#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <utility>

namespace huxerui::interaction {
namespace detail {
struct RadialDialControllerState;
} // namespace detail

struct RadialDialSnapshot;
struct RadialDialStyle;

/// Value-to-angle mapping mode for RadialDial.
enum class RadialDialMode {
  /// Clamps rotation to `[min_value, max_value]` across `[start_angle_degrees, start_angle_degrees + sweep_degrees]`.
  Bounded,
  /// Supports unlimited multi-turn rotation mapping `value_per_turn` units per full `360°` revolution.
  Continuous,
};

/// Live angle, turn count, normalized fraction, and interaction flags passed to headless RadialDial builders.
struct RadialDialSnapshot {
  /// Current clamped and quantized numeric value.
  double value = 0.0;
  /// Normalized progress in `[0, 1]` (`[min_value, max_value]` in Bounded mode; fractional turn in Continuous mode).
  float normalized = 0.0F;
  /// Current pointer indicator angle in degrees (`0°` East, increasing clockwise).
  double angle_degrees = 135.0;
  /// Total angular travel in full `360°` turns (`value / value_per_turn` in Continuous mode).
  double turns = 0.0;
  /// True while a pointer drag sequence is active.
  bool dragging = false;
  /// True while `Shift` fine precision is active during dragging.
  bool fine_precision = false;
  /// True while a pointer hovers the dial.
  bool hovered = false;
  /// True while the dial holds keyboard focus.
  bool focused = false;

  bool operator==(const RadialDialSnapshot&) const = default;
};

/// Copyable handle for reading live dial values/angles and driving programmatic adjustments.
/// Default-constructed handles are unbound and safe outside an Application runtime; bind one with
/// UseRadialDialController().
class RadialDialController {
public:
  RadialDialController() noexcept = default;

  [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(state_); }

  /// Current numeric value (subscribes active composition). Returns `0.0` when unbound.
  [[nodiscard]] double Value() const;

  /// Normalized `[0, 1]` ratio (subscribes active composition).
  [[nodiscard]] float Normalized() const;

  /// Current indicator angle in degrees (subscribes active composition). Returns `135.0` when unbound.
  [[nodiscard]] double AngleDegrees() const;

  /// True while the user is dragging the dial (subscribes active composition).
  [[nodiscard]] bool IsDragging() const;

  /// Clamps, quantizes (if `step` is set), and commits `value`, firing `on_change` / `on_commit` when changed.
  void SetValue(double value) const;

  /// Adds `delta` to the current value using the bound clamping and quantization rules.
  void Nudge(double delta) const;

  /// Resets the dial to `default_value` (or `initial_value` when `default_value` is unset).
  void Reset() const;

  bool operator==(const RadialDialController&) const = default;

private:
  explicit RadialDialController(std::shared_ptr<detail::RadialDialControllerState> state)
      : state_(std::move(state)) {}

  std::shared_ptr<detail::RadialDialControllerState> state_;

  friend RadialDialController UseRadialDialController(double initial_value);
  friend View RadialDial(std::function<View(RadialDialSnapshot)> builder, RadialDialStyle style,
                         std::function<void(double value)> on_change,
                         std::function<void(double value)> on_commit);
  friend View RadialDial(RadialDialStyle style, std::function<void(double value)> on_change,
                         std::function<void(double value)> on_commit);
};

/// Remembers a RadialDialController in the current composition scope.
/// @throws std::invalid_argument If `initial_value` is non-finite.
RadialDialController UseRadialDialController(double initial_value = 0.0);

/// Mode, bounds, arc geometry, wheel/keyboard steps, visual styling, and controller configuration for RadialDial.
struct RadialDialStyle {
  /// Clamped bounded arc vs. unbounded multi-turn rotary encoder.
  RadialDialMode mode = RadialDialMode::Bounded;
  /// Initial value applied on mount (and fallback target for Reset()). Must be finite and in `[min_value, max_value]`.
  double initial_value = 0.0;
  /// Optional reset target used on double-click or Reset(). When set, must be finite and in `[min_value, max_value]`.
  std::optional<double> default_value{};
  /// Inclusive lower bound (must be finite when `mode == Bounded`; may be `-infinity` in `Continuous`).
  double min_value = 0.0;
  /// Inclusive upper bound (must be finite and `> min_value` in `Bounded`; may be `+infinity` in `Continuous`).
  double max_value = 100.0;
  /// Starting indicator angle in degrees (`0°` East, increasing clockwise). Default `135°` (7:30 o'clock).
  double start_angle_degrees = 135.0;
  /// Total clockwise sweep in degrees when `mode == Bounded`. Must be finite and in `(0, 360]`.
  double sweep_degrees = 270.0;
  /// Value change per full `360°` revolution when `mode == Continuous`. Must be finite and positive.
  double value_per_turn = 360.0;
  /// Optional quantization step anchored at `min_value` (if finite) or `0.0`. Must be finite and positive.
  std::optional<double> step{};
  /// Keyboard increment per arrow press when `step` is unset. Must be finite and positive.
  double keyboard_step = 5.0;
  /// Multiplier applied when holding `Shift` during pointer drag, wheel, or keyboard adjustment. Must be finite > 0.
  double fine_multiplier = 0.2;
  /// When true in `Bounded` mode, pointer press immediately jumps to the pointer angle instead of relative turning.
  bool jump_to_pointer_on_down = false;
  /// Inner radius around the dial center in logical pixels where angular tracking pauses to prevent jitter. Must be >= 0.
  float center_dead_zone = 6.0F;
  /// When true, mouse wheel / trackpad scroll over the dial adjusts the value.
  bool wheel_enabled = true;
  /// Value change per standard wheel notch (`120` scroll units). Must be finite and positive.
  double wheel_step = 5.0;
  /// When true, double-clicking resets to `default_value.value_or(initial_value)`.
  bool double_click_resets = true;
  /// Whether the dial participates in keyboard focus and arrow/Home/End/PageUp/PageDown keys.
  bool keyboard_focusable = true;
  /// Default diameter used by the preset RadialDial overload in logical pixels. Must be finite and positive.
  float diameter = 72.0F;
  /// Stroke width of the track and active arcs in the preset overload in logical pixels. Must be finite and positive.
  float track_thickness = 6.0F;
  /// Background arc color in the preset overload.
  Color track_color = Color::Rgb(0, 0, 0, 0.14F);
  /// Active sweep arc, pointer needle, and focus halo color in the preset overload.
  Color active_color = Color::Rgb(31, 111, 235, 1.0F);
  /// Center knob fill color in the preset overload.
  Color knob_color = Color::Rgb(245, 247, 250, 1.0F);
  /// Optional external controller bound to this RadialDial.
  RadialDialController controller{};

  bool operator==(const RadialDialStyle&) const = default;
};

/// Headless rotary dial primitive delegating visual presentation to `builder(RadialDialSnapshot)`.
/// @throws std::invalid_argument If `builder` is empty or `style` contains invalid bounds or geometry.
View RadialDial(std::function<View(RadialDialSnapshot)> builder, RadialDialStyle style = {},
                std::function<void(double value)> on_change = {},
                std::function<void(double value)> on_commit = {});

/// Preset painted rotary dial with rounded active sweep arc and center needle knob.
/// @throws std::invalid_argument If `style` contains invalid bounds or geometry.
View RadialDial(RadialDialStyle style = {}, std::function<void(double value)> on_change = {},
                std::function<void(double value)> on_commit = {});

} // namespace huxerui::interaction
