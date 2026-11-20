#include <huxerui/interaction/gesture/radialdial.h>

#include "../internal/validate.h"

#include <huxerui/layout.h>
#include <huxerui/lifecycle.h>
#include <huxerui/modifier.h>
#include <huxerui/paint.h>
#include <huxerui/semantics.h>
#include <huxerui/state.h>
#include <huxerui/task.h>
#include <huxerui/vector.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {

namespace detail {

struct RadialDialControllerState {
  State<double> value;
  State<float> normalized;
  State<double> angle_degrees;
  State<double> turns;
  State<bool> dragging;
  State<bool> fine_precision;
  State<bool> hovered;
  State<bool> focused;
  RadialDialStyle style;
  std::function<void(double)> on_change;
  std::function<void(double)> on_commit;

  bool initialized = false;
  bool has_explicit_controller_initial = false;
  bool sync_scheduled = false;
  bool needs_sync = false;
  bool enabled = true;

  std::shared_ptr<bool> alive;
  std::function<void()> post_sync;
  std::function<void(double value)> set_value_fn;
};

} // namespace detail

namespace {

constexpr double kValueEpsilon = 1e-12;
constexpr float kMoveSlop = 4.0F;
constexpr float kDoubleClickMaxDistance = 10.0F;
constexpr auto kDoubleClickInterval = std::chrono::duration<double>(0.35);
constexpr double kDegToRad = std::numbers::pi / 180.0;
constexpr double kRadToDeg = 180.0 / std::numbers::pi;

void ValidateStyle(const RadialDialStyle& style) {
  if(style.mode == RadialDialMode::Bounded) {
    internal::RequireFinite(style.min_value,
                            "HuxerUI Interaction: RadialDial Bounded min_value must be finite");
    internal::RequireFinite(style.max_value,
                            "HuxerUI Interaction: RadialDial Bounded max_value must be finite");
    if(style.min_value >= style.max_value) {
      throw std::invalid_argument(
          "HuxerUI Interaction: RadialDial Bounded mode requires min_value < max_value");
    }
    internal::RequirePositive(style.sweep_degrees,
                              "HuxerUI Interaction: RadialDial Bounded sweep_degrees must be finite and positive");
    if(style.sweep_degrees > 360.0) {
      throw std::invalid_argument(
          "HuxerUI Interaction: RadialDial sweep_degrees must not exceed 360");
    }
  } else {
    if(std::isnan(style.min_value) || style.min_value == std::numeric_limits<double>::infinity()) {
      throw std::invalid_argument(
          "HuxerUI Interaction: RadialDial min_value must not be NaN or +infinity");
    }
    if(std::isnan(style.max_value) || style.max_value == -std::numeric_limits<double>::infinity()) {
      throw std::invalid_argument(
          "HuxerUI Interaction: RadialDial max_value must not be NaN or -infinity");
    }
    if(style.min_value > style.max_value) {
      throw std::invalid_argument("HuxerUI Interaction: RadialDial min_value must not exceed max_value");
    }
    internal::RequirePositive(style.value_per_turn,
                              "HuxerUI Interaction: RadialDial value_per_turn must be finite and positive");
  }

  internal::RequireFinite(style.initial_value,
                          "HuxerUI Interaction: RadialDial initial_value must be finite");
  if(style.initial_value < style.min_value || style.initial_value > style.max_value) {
    throw std::invalid_argument(
        "HuxerUI Interaction: RadialDial initial_value must lie within [min_value, max_value]");
  }
  if(style.default_value.has_value()) {
    internal::RequireFinite(*style.default_value,
                            "HuxerUI Interaction: RadialDial default_value must be finite");
    if(*style.default_value < style.min_value || *style.default_value > style.max_value) {
      throw std::invalid_argument(
          "HuxerUI Interaction: RadialDial default_value must lie within [min_value, max_value]");
    }
  }
  internal::RequireFinite(style.start_angle_degrees,
                          "HuxerUI Interaction: RadialDial start_angle_degrees must be finite");
  if(style.step.has_value()) {
    internal::RequirePositive(*style.step, "HuxerUI Interaction: RadialDial step must be finite and positive");
  }
  internal::RequirePositive(style.keyboard_step,
                            "HuxerUI Interaction: RadialDial keyboard_step must be finite and positive");
  internal::RequirePositive(style.fine_multiplier,
                            "HuxerUI Interaction: RadialDial fine_multiplier must be finite and positive");
  internal::RequireNonNegative(style.center_dead_zone,
                               "HuxerUI Interaction: RadialDial center_dead_zone must be finite and non-negative");
  internal::RequirePositive(style.wheel_step,
                            "HuxerUI Interaction: RadialDial wheel_step must be finite and positive");
  internal::RequirePositive(style.diameter,
                            "HuxerUI Interaction: RadialDial diameter must be finite and positive");
  internal::RequirePositive(style.track_thickness,
                            "HuxerUI Interaction: RadialDial track_thickness must be finite and positive");
}

double ClampAndQuantize(const RadialDialStyle& style, double raw) noexcept {
  if(!std::isfinite(raw)) {
    raw = style.initial_value;
  }
  double v = std::clamp(raw, style.min_value, style.max_value);
  if(style.step.has_value()) {
    const double step = *style.step;
    const double anchor = std::isfinite(style.min_value) ? style.min_value : 0.0;
    v = anchor + std::round((v - anchor) / step) * step;
    v = std::clamp(v, style.min_value, style.max_value);
  }
  if(std::abs(v) < kValueEpsilon) {
    v = 0.0;
  }
  return v;
}

struct DialTelemetry {
  float normalized = 0.0F;
  double angle_degrees = 135.0;
  double turns = 0.0;
};

DialTelemetry ComputeTelemetry(const RadialDialStyle& style, double value) noexcept {
  if(style.mode == RadialDialMode::Bounded) {
    const double span = style.max_value - style.min_value;
    const double norm = span > kValueEpsilon ? std::clamp((value - style.min_value) / span, 0.0, 1.0) : 0.0;
    const double sweep = norm * style.sweep_degrees;
    return {
        .normalized = static_cast<float>(norm),
        .angle_degrees = style.start_angle_degrees + sweep,
        .turns = sweep / 360.0,
    };
  }
  const double turns = value / style.value_per_turn;
  const double frac = turns - std::floor(turns);
  return {
      .normalized = static_cast<float>(frac),
      .angle_degrees = style.start_angle_degrees + turns * 360.0,
      .turns = turns,
  };
}

double ShortestSignedAngleDelta(double from_deg, double to_deg) noexcept {
  double diff = std::fmod(to_deg - from_deg, 360.0);
  if(diff > 180.0) diff -= 360.0;
  if(diff <= -180.0) diff += 360.0;
  return diff;
}

void ScheduleSync(detail::RadialDialControllerState& state) {
  if(state.sync_scheduled || !state.alive || !*state.alive || !state.post_sync) return;
  state.sync_scheduled = true;
  state.post_sync();
}

class RadialDialPassThroughLayout final : public Layout<RadialDialPassThroughLayout> {
public:
  using Layout::Layout;

  static LayoutResult Measure(LayoutContext& context, ViewNode& node, const Constraints& constraints) {
    LayoutResult result;
    if(node.ChildCount() == 0U) {
      return result.SetSize(constraints.Constrain(Size{}));
    }
    const Size size = context.Measure(node.ChildAt(0U), constraints);
    result.Place(node.ChildAt(0U), Point{0.0F, 0.0F});
    return result.SetSize(constraints.Constrain(size));
  }
};

struct RadialDialModifier {
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  std::shared_ptr<detail::RadialDialControllerState> state;
  TaskScope tasks{};
  double value = 0.0;
  double angle_degrees = 135.0;
  RadialDialMode mode = RadialDialMode::Bounded;
  double min_value = 0.0;
  double max_value = 100.0;
  bool has_builder = false;

  bool operator==(const RadialDialModifier& other) const noexcept {
    return state == other.state && value == other.value && angle_degrees == other.angle_degrees && mode == other.mode
           && min_value == other.min_value && max_value == other.max_value && has_builder == other.has_builder;
  }
};

class RadialDialModifier::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const RadialDialModifier& value) { Update(node, value); }

  ~Extension() override {
    ++click_token_;
    if(state_) {
      state_->set_value_fn = {};
    }
  }

  void Update(ViewNode&, const RadialDialModifier& value) {
    const bool state_changed = state_ != value.state;
    if(state_ && state_changed) {
      state_->set_value_fn = {};
      pointer_id_.reset();
      has_last_angle_ = false;
      moved_during_press_ = false;
      ++click_token_;
      pending_click_ = false;
    }
    state_ = value.state;
    tasks_ = value.tasks;
    has_builder_ = value.has_builder;
    if(state_) {
      state_->set_value_fn = [this](double target) { ProgrammaticSetValue(target); };
      if(state_changed) {
        raw_value_ = CurrentValue();
        state_->needs_sync = true;
      }
    }
    InvalidatePaint(PaintInvalidation::Foreground);
  }

  [[nodiscard]] bool HitTest(ViewNode& node, Point position) const override {
    return node.IsEnabled() && node.Bounds().Contains(position);
  }

  [[nodiscard]] bool HoverHitTest(ViewNode& node, Point position) const override {
    return HitTest(node, position);
  }

  void OnHover(ViewNode&, const HoverEvent& event) override {
    if(!state_) return;
    const bool is_hovered = event.type != HoverEventType::Leave;
    if(state_->hovered.IsValid() && state_->hovered.Get() != is_hovered) {
      state_->hovered = is_hovered;
    }
    InvalidatePaint(PaintInvalidation::Foreground);
  }

  void OnFocusChanged(ViewNode&, bool focused, bool) override {
    if(!state_) return;
    if(state_->focused.IsValid() && state_->focused.Get() != focused) {
      state_->focused = focused;
    }
    InvalidatePaint(PaintInvalidation::Foreground);
  }

  void OnInteraction(ViewNode&, const InteractionState& state, const std::optional<InteractionEvent>&) override {
    if(!state_) return;
    state_->enabled = state.enabled;
    if(!state.enabled && pointer_id_.has_value()) {
      pointer_id_.reset();
      has_last_angle_ = false;
      moved_during_press_ = false;
      ++click_token_;
      pending_click_ = false;
      state_->needs_sync = true;
      ScheduleSync(*state_);
    }
  }

  PointerResult OnPointer(ViewNode& node, const PointerEvent& event) override {
    if(!state_ || !node.IsEnabled() || !state_->enabled) {
      if(pointer_id_.has_value()) {
        CancelActiveDrag(false);
      }
      return PointerResult::Ignored;
    }

    const Rect bounds = node.Bounds();
    const Point center{bounds.x + bounds.width * 0.5F, bounds.y + bounds.height * 0.5F};

    if(event.type == PointerEventType::Down) {
      if(event.changed_button != PointerButton::Primary || pointer_id_.has_value()) {
        return PointerResult::Ignored;
      }
      pointer_id_ = event.pointer_id;
      down_position_ = event.position;
      drag_start_value_ = ClampAndQuantize(state_->style, CurrentValue());
      raw_value_ = drag_start_value_;
      moved_during_press_ = false;
      has_last_angle_ = false;

      const double dx = static_cast<double>(event.position.x - center.x);
      const double dy = static_cast<double>(event.position.y - center.y);
      const double dist = std::hypot(dx, dy);
      if(dist >= static_cast<double>(state_->style.center_dead_zone)) {
        const double angle = std::atan2(dy, dx) * kRadToDeg;
        last_pointer_angle_ = angle;
        has_last_angle_ = true;
        if(state_->style.jump_to_pointer_on_down && state_->style.mode == RadialDialMode::Bounded) {
          ++click_token_;
          pending_click_ = false;
          const double jumped = BoundedAngleToValue(angle);
          raw_value_ = jumped;
          ApplyValue(ClampAndQuantize(state_->style, raw_value_), true, false);
        }
      }
      SetDragStates(true, event.modifiers.shift);
      InvalidatePaint(PaintInvalidation::Foreground);
      return PointerResult::Capture;
    }

    if(!pointer_id_.has_value() || *pointer_id_ != event.pointer_id) {
      return PointerResult::Ignored;
    }

    if(event.type == PointerEventType::Move) {
      if(std::hypot(event.position.x - down_position_.x, event.position.y - down_position_.y) > kMoveSlop) {
        moved_during_press_ = true;
        ++click_token_;
        pending_click_ = false;
      }

      const double dx = static_cast<double>(event.position.x - center.x);
      const double dy = static_cast<double>(event.position.y - center.y);
      const double dist = std::hypot(dx, dy);
      const bool fine = event.modifiers.shift;
      if(dist >= static_cast<double>(state_->style.center_dead_zone)) {
        const double angle = std::atan2(dy, dx) * kRadToDeg;
        if(!has_last_angle_) {
          last_pointer_angle_ = angle;
          has_last_angle_ = true;
        } else {
          const double delta_deg = ShortestSignedAngleDelta(last_pointer_angle_, angle);
          last_pointer_angle_ = angle;
          if(std::abs(delta_deg) > 1e-7) {
            const double mult = fine ? state_->style.fine_multiplier : 1.0;
            double value_delta = 0.0;
            if(state_->style.mode == RadialDialMode::Bounded) {
              const double span = state_->style.max_value - state_->style.min_value;
              value_delta = (delta_deg / state_->style.sweep_degrees) * span * mult;
            } else {
              value_delta = (delta_deg / 360.0) * state_->style.value_per_turn * mult;
            }
            raw_value_ = std::clamp(raw_value_ + value_delta, state_->style.min_value, state_->style.max_value);
            const double next = ClampAndQuantize(state_->style, raw_value_);
            ApplyValue(next, true, false);
          }
        }
      }
      SetDragStates(true, fine);
      InvalidatePaint(PaintInvalidation::Foreground);
      return PointerResult::Handled;
    }

    if(event.type == PointerEventType::Up) {
      if(std::hypot(event.position.x - down_position_.x, event.position.y - down_position_.y) > kMoveSlop) {
        moved_during_press_ = true;
      }
      const bool was_moved = moved_during_press_;
      pointer_id_.reset();
      has_last_angle_ = false;
      moved_during_press_ = false;
      SetDragStates(false, false);

      const double final_value = ClampAndQuantize(state_->style, CurrentValue());
      const bool value_changed = std::abs(final_value - drag_start_value_) > kValueEpsilon;
      if(was_moved || value_changed) {
        if(value_changed && state_->on_commit) {
          auto cb = state_->on_commit;
          cb(final_value);
        }
      } else if(state_->style.double_click_resets) {
        HandleCleanClick(event.position);
      }
      InvalidatePaint(PaintInvalidation::Foreground);
      return PointerResult::Handled;
    }

    if(event.type == PointerEventType::Cancel) {
      CancelActiveDrag(true);
      InvalidatePaint(PaintInvalidation::Foreground);
      return PointerResult::Handled;
    }

    return PointerResult::Ignored;
  }

  [[nodiscard]] bool OnKey(ViewNode& node, const KeyEvent& event) override {
    if(!state_ || !node.IsEnabled() || !state_->enabled || !state_->style.keyboard_focusable
       || event.type != KeyEventType::Down) {
      return false;
    }
    if(event.key == Key::Escape && pointer_id_.has_value()) {
      CancelActiveDrag(true);
      InvalidatePaint(PaintInvalidation::Foreground);
      return true;
    }
    if(pointer_id_.has_value() || event.modifiers.alt || event.modifiers.control || event.modifiers.meta) {
      return false;
    }

    const double base = state_->style.step.value_or(state_->style.keyboard_step);
    const double mult = event.modifiers.shift ? state_->style.fine_multiplier : 1.0;
    const double step_delta = base * mult;
    const double page_delta = base * 5.0 * mult;
    const double current = ClampAndQuantize(state_->style, CurrentValue());

    std::optional<double> next;
    switch(event.key) {
    case Key::ArrowRight:
    case Key::ArrowUp:
      next = current + step_delta;
      break;
    case Key::ArrowLeft:
    case Key::ArrowDown:
      next = current - step_delta;
      break;
    case Key::PageUp:
      next = current + page_delta;
      break;
    case Key::PageDown:
      next = current - page_delta;
      break;
    case Key::Home:
      if(std::isfinite(state_->style.min_value)) {
        next = state_->style.min_value;
      }
      break;
    case Key::End:
      if(std::isfinite(state_->style.max_value)) {
        next = state_->style.max_value;
      }
      break;
    default:
      return false;
    }
    if(!next.has_value()) return false;
    const double committed = ClampAndQuantize(state_->style, *next);
    raw_value_ = committed;
    ApplyValue(committed, true, true);
    InvalidatePaint(PaintInvalidation::Foreground);
    return true;
  }

  void BuildSemantics(SemanticBuilder& builder) const override {
    if(!state_) return;
    const double current = ClampAndQuantize(state_->style, CurrentValue());
    std::optional<SemanticRange> range;
    if(std::isfinite(state_->style.min_value) && std::isfinite(state_->style.max_value)) {
      range = SemanticRange{
          .minimum = state_->style.min_value,
          .maximum = state_->style.max_value,
          .current = current,
          .step = state_->style.step.value_or(state_->style.keyboard_step),
      };
    }
    builder.SetOwner(Semantics{
        .role = SemanticRole::Slider,
        .range = range,
    });
    builder.AddAction(0, SemanticActionKind::Increment);
    builder.AddAction(0, SemanticActionKind::Decrement);
  }

  [[nodiscard]] bool OnSemanticAction(std::uint64_t local_id, const SemanticAction& action) override {
    if(local_id != 0 || !state_ || !state_->enabled) return false;
    const double base = state_->style.step.value_or(state_->style.keyboard_step);
    const double current = ClampAndQuantize(state_->style, CurrentValue());
    if(action.kind == SemanticActionKind::Increment) {
      const double next = ClampAndQuantize(state_->style, current + base);
      raw_value_ = next;
      ApplyValue(next, true, true);
      InvalidatePaint(PaintInvalidation::Foreground);
      return true;
    }
    if(action.kind == SemanticActionKind::Decrement) {
      const double next = ClampAndQuantize(state_->style, current - base);
      raw_value_ = next;
      ApplyValue(next, true, true);
      InvalidatePaint(PaintInvalidation::Foreground);
      return true;
    }
    return false;
  }

  PaintInvalidation PrepareGeometry(ViewNode&, TextMeasurer&) override {
    if(!state_) return PaintInvalidation::None;
    const double cur = CurrentValue();
    const double clamped = ClampAndQuantize(state_->style, cur);
    if(state_->needs_sync || !state_->initialized || std::abs(cur - clamped) > kValueEpsilon) {
      state_->needs_sync = false;
      ScheduleSync(*state_);
    }
    return PaintInvalidation::None;
  }

  void PaintAboveContent(const ViewNode& node, PaintContext& context) const override {
    if(has_builder_ || !state_) return;
    const Rect frame = node.Bounds();
    if(frame.width <= 0.0F || frame.height <= 0.0F) return;

    const RadialDialStyle& style = state_->style;
    const double value = ClampAndQuantize(style, CurrentValue());
    const DialTelemetry tel = ComputeTelemetry(style, value);

    const Point center{frame.x + frame.width * 0.5F, frame.y + frame.height * 0.5F};
    const float outer_r = std::min(frame.width, frame.height) * 0.5F;
    const float stroke_w = std::min(style.track_thickness, std::max(1.0F, outer_r * 0.5F));
    const float arc_r = std::max(0.0F, outer_r - stroke_w * 0.5F - 2.0F);
    if(arc_r <= 0.0F) return;

    const StrokeStyle arc_stroke{.width = stroke_w, .cap = StrokeCap::Round};
    const float start_rad = static_cast<float>(style.start_angle_degrees * kDegToRad);
    const float track_sweep =
        style.mode == RadialDialMode::Bounded ? static_cast<float>(style.sweep_degrees * kDegToRad)
                                              : static_cast<float>(2.0 * std::numbers::pi);

    if(style.track_color.alpha > 0.0F) {
      context.DrawArc(center, arc_r, start_rad, track_sweep, style.track_color, arc_stroke);
    }

    if(style.active_color.alpha > 0.0F) {
      if(style.mode == RadialDialMode::Bounded) {
        const float active_sweep = static_cast<float>(static_cast<double>(tel.normalized) * style.sweep_degrees * kDegToRad);
        if(std::abs(active_sweep) > 1e-4F) {
          context.DrawArc(center, arc_r, start_rad, active_sweep, style.active_color, arc_stroke);
        }
      } else {
        const float active_sweep = static_cast<float>(static_cast<double>(tel.normalized) * 2.0 * std::numbers::pi);
        if(std::abs(active_sweep) > 1e-4F) {
          context.DrawArc(center, arc_r, start_rad, active_sweep, style.active_color, arc_stroke);
        }
      }
    }

    const float knob_r = std::max(0.0F, arc_r - stroke_w * 0.65F - 3.0F);
    if(knob_r <= 0.0F) return;

    const bool hovered = state_->hovered.IsValid() && state_->hovered.Get();
    const bool focused = state_->focused.IsValid() && state_->focused.Get();
    const bool dragging = state_->dragging.IsValid() && state_->dragging.Get();

    if((hovered || focused || dragging) && style.active_color.alpha > 0.0F) {
      const Color halo{
          style.active_color.red,
          style.active_color.green,
          style.active_color.blue,
          dragging ? 0.28F : 0.16F,
      };
      context.DrawCircle(center, knob_r + 3.0F, halo);
    }

    if(style.knob_color.alpha > 0.0F) {
      context.DrawCircle(center, knob_r, style.knob_color);
    }

    if(style.active_color.alpha > 0.0F && knob_r >= 4.0F) {
      const double rad = tel.angle_degrees * kDegToRad;
      const float ux = static_cast<float>(std::cos(rad));
      const float uy = static_cast<float>(std::sin(rad));
      const float r_inner = knob_r * 0.28F;
      const float r_outer = std::max(r_inner, knob_r - 3.0F);
      const Point p0{center.x + ux * r_inner, center.y + uy * r_inner};
      const Point p1{center.x + ux * r_outer, center.y + uy * r_outer};
      context.DrawLine(p0, p1, style.active_color,
                       StrokeStyle{.width = std::max(2.0F, stroke_w * 0.45F), .cap = StrokeCap::Round});
    }
  }

private:
  [[nodiscard]] double CurrentValue() const {
    if(!state_) return 0.0;
    if(state_->value.IsValid()) return state_->value.Get();
    return state_->style.initial_value;
  }

  [[nodiscard]] double BoundedAngleToValue(double angle_deg) const noexcept {
    const RadialDialStyle& style = state_->style;
    double offset = std::fmod(angle_deg - style.start_angle_degrees, 360.0);
    if(offset < 0.0) offset += 360.0;
    if(offset > style.sweep_degrees) {
      const double half_gap = (360.0 - style.sweep_degrees) * 0.5;
      offset = (offset > style.sweep_degrees + half_gap) ? 0.0 : style.sweep_degrees;
    }
    const double norm = style.sweep_degrees > 1e-6 ? (offset / style.sweep_degrees) : 0.0;
    return style.min_value + norm * (style.max_value - style.min_value);
  }

  void SetDragStates(bool dragging, bool fine) {
    if(!state_) return;
    if(state_->dragging.IsValid() && state_->dragging.Get() != dragging) {
      state_->dragging = dragging;
    }
    if(state_->fine_precision.IsValid() && state_->fine_precision.Get() != fine) {
      state_->fine_precision = fine;
    }
  }

  void ApplyValue(double next, bool notify_change, bool notify_commit) {
    if(!state_ || !state_->value.IsValid()) return;
    const double prev = state_->value.Get();
    const DialTelemetry tel = ComputeTelemetry(state_->style, next);
    if(state_->normalized.IsValid() && std::abs(state_->normalized.Get() - tel.normalized) > 1e-6F) {
      state_->normalized = tel.normalized;
    }
    if(state_->angle_degrees.IsValid() && std::abs(state_->angle_degrees.Get() - tel.angle_degrees) > 1e-6) {
      state_->angle_degrees = tel.angle_degrees;
    }
    if(state_->turns.IsValid() && std::abs(state_->turns.Get() - tel.turns) > 1e-9) {
      state_->turns = tel.turns;
    }
    if(std::abs(next - prev) <= kValueEpsilon && next == prev) return;
    state_->value = next;
    const bool changed = std::abs(next - prev) > kValueEpsilon;
    if(changed && notify_change && state_->on_change) {
      auto cb = state_->on_change;
      cb(next);
    }
    if(changed && notify_commit && state_->on_commit) {
      auto cb = state_->on_commit;
      cb(next);
    }
  }

  void ProgrammaticSetValue(double target) {
    if(!state_) return;
    const double next = ClampAndQuantize(state_->style, target);
    raw_value_ = next;
    ApplyValue(next, true, true);
    InvalidatePaint(PaintInvalidation::Foreground);
  }

  void CancelActiveDrag(bool restore_value) {
    pointer_id_.reset();
    has_last_angle_ = false;
    moved_during_press_ = false;
    ++click_token_;
    pending_click_ = false;
    if(!state_) return;
    SetDragStates(false, false);
    if(restore_value) {
      const double restored = ClampAndQuantize(state_->style, drag_start_value_);
      raw_value_ = restored;
      ApplyValue(restored, true, false);
    }
  }

  void HandleCleanClick(Point position) {
    if(!state_) return;
    if(pending_click_
       && std::hypot(position.x - pending_click_pos_.x, position.y - pending_click_pos_.y) <= kDoubleClickMaxDistance) {
      pending_click_ = false;
      ++click_token_;
      const double target =
          ClampAndQuantize(state_->style, state_->style.default_value.value_or(state_->style.initial_value));
      raw_value_ = target;
      ApplyValue(target, true, true);
      return;
    }
    pending_click_ = true;
    pending_click_pos_ = position;
    const std::uint64_t token = ++click_token_;
    const std::shared_ptr<bool> alive = state_->alive;
    std::weak_ptr<detail::RadialDialControllerState> weak_state = state_;
    tasks_.Launch([this, alive, weak_state, token]() -> Task<void> {
      co_await Delay(kDoubleClickInterval);
      if(!alive || !*alive || weak_state.expired() || click_token_ != token) {
        co_return;
      }
      pending_click_ = false;
    });
  }

  std::shared_ptr<detail::RadialDialControllerState> state_;
  TaskScope tasks_{};
  bool has_builder_ = false;

  std::optional<std::int64_t> pointer_id_{};
  Point down_position_{};
  double drag_start_value_ = 0.0;
  double raw_value_ = 0.0;
  double last_pointer_angle_ = 0.0;
  bool has_last_angle_ = false;
  bool moved_during_press_ = false;

  bool pending_click_ = false;
  Point pending_click_pos_{};
  std::uint64_t click_token_ = 0;
};

const huxerui::detail::ModifierDescriptor& RadialDialModifier::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<RadialDialModifier, RadialDialModifier::Extension>();
}

View BuildRadialDialImpl(std::function<View(RadialDialSnapshot)> builder, RadialDialStyle style,
                         std::function<void(double value)> on_change, std::function<void(double value)> on_commit,
                         std::shared_ptr<detail::RadialDialControllerState> external_state) {
  ValidateStyle(style);
  const bool has_builder = static_cast<bool>(builder);
  const double init_val = ClampAndQuantize(style, style.initial_value);
  const DialTelemetry init_tel = ComputeTelemetry(style, init_val);

  return Scope([builder = std::move(builder), style = std::move(style), on_change = std::move(on_change),
                on_commit = std::move(on_commit), external_state = std::move(external_state), has_builder, init_val,
                init_tel]() mutable -> View {
    TaskScope tasks = UseTaskScope();
    auto alive = UseState(std::make_shared<bool>(true)).Get();
    auto local_value = UseState(init_val);
    auto local_norm = UseState(init_tel.normalized);
    auto local_angle = UseState(init_tel.angle_degrees);
    auto local_turns = UseState(init_tel.turns);
    auto local_dragging = UseState(false);
    auto local_fine = UseState(false);
    auto local_hovered = UseState(false);
    auto local_focused = UseState(false);
    auto local_holder = UseState(std::make_shared<detail::RadialDialControllerState>());

    std::shared_ptr<detail::RadialDialControllerState> state = external_state ? external_state : local_holder.Get();
    if(!external_state) {
      state->value = local_value;
      state->normalized = local_norm;
      state->angle_degrees = local_angle;
      state->turns = local_turns;
      state->dragging = local_dragging;
      state->fine_precision = local_fine;
      state->hovered = local_hovered;
      state->focused = local_focused;
    }

    state->style = style;
    state->style.controller = {};
    state->on_change = on_change;
    state->on_commit = on_commit;
    state->alive = alive;
    *alive = true;

    std::weak_ptr<detail::RadialDialControllerState> weak_state = state;
    state->post_sync = [tasks, weak_state, alive]() {
      tasks.Post([weak_state, alive]() {
        if(!*alive) return;
        auto locked = weak_state.lock();
        if(!locked) return;
        locked->sync_scheduled = false;

        if(!locked->enabled) {
          if(locked->dragging.IsValid() && locked->dragging.Get()) locked->dragging = false;
          if(locked->fine_precision.IsValid() && locked->fine_precision.Get()) locked->fine_precision = false;
        }
        if(!locked->initialized) {
          locked->initialized = true;
          if(!locked->has_explicit_controller_initial && locked->value.IsValid()) {
            const double target = ClampAndQuantize(locked->style, locked->style.initial_value);
            if(std::abs(locked->value.Get() - target) > kValueEpsilon) {
              locked->value = target;
            }
          }
        }
        if(locked->value.IsValid()) {
          const double cur = locked->value.Get();
          const double clamped = ClampAndQuantize(locked->style, cur);
          if(std::abs(cur - clamped) > kValueEpsilon) {
            locked->value = clamped;
          }
          const DialTelemetry tel = ComputeTelemetry(locked->style, clamped);
          if(locked->normalized.IsValid() && std::abs(locked->normalized.Get() - tel.normalized) > 1e-6F) {
            locked->normalized = tel.normalized;
          }
          if(locked->angle_degrees.IsValid() && std::abs(locked->angle_degrees.Get() - tel.angle_degrees) > 1e-6) {
            locked->angle_degrees = tel.angle_degrees;
          }
          if(locked->turns.IsValid() && std::abs(locked->turns.Get() - tel.turns) > 1e-9) {
            locked->turns = tel.turns;
          }
        }
      });
    };

    Lifecycle(
        [weak_state, alive]() {
          *alive = true;
          return [weak_state, alive]() {
            *alive = false;
            if(auto locked = weak_state.lock()) {
              locked->post_sync = {};
              locked->sync_scheduled = false;
            }
          };
        },
        state);

    return Scope([builder, state, tasks, has_builder]() -> View {
      const double raw = state->value.IsValid() ? state->value.Get() : state->style.initial_value;
      const double value = ClampAndQuantize(state->style, raw);
      const DialTelemetry tel = ComputeTelemetry(state->style, value);
      const bool dragging = state->dragging.IsValid() && state->dragging.Get();
      const bool fine = state->fine_precision.IsValid() && state->fine_precision.Get();
      const bool hovered = state->hovered.IsValid() && state->hovered.Get();
      const bool focused = state->focused.IsValid() && state->focused.Get();

      View child = has_builder ? builder(RadialDialSnapshot{
                                     .value = value,
                                     .normalized = tel.normalized,
                                     .angle_degrees = tel.angle_degrees,
                                     .turns = tel.turns,
                                     .dragging = dragging,
                                     .fine_precision = fine,
                                     .hovered = hovered,
                                     .focused = focused,
                                 })
                               : Spacer{}.With(Frame{.width = state->style.diameter, .height = state->style.diameter});

      return RadialDialPassThroughLayout(std::move(child))
          .With(Focusable{state->style.keyboard_focusable}, PointerCursor{PointerCursorKind::Hand},
                RadialDialModifier{
                    .state = state,
                    .tasks = tasks,
                    .value = value,
                    .angle_degrees = tel.angle_degrees,
                    .mode = state->style.mode,
                    .min_value = state->style.min_value,
                    .max_value = state->style.max_value,
                    .has_builder = has_builder,
                })
          .On<ViewEvents::ScrollInput>([state](const ScrollInputEvent& event) -> bool {
            if(!state || !state->enabled || !state->style.wheel_enabled) return false;
            const double dir = std::abs(event.delta_y) >= std::abs(event.delta_x)
                                   ? -static_cast<double>(event.delta_y)
                                   : static_cast<double>(event.delta_x);
            if(std::abs(dir) <= 1e-6) return false;
            const double mult = event.modifiers.shift ? state->style.fine_multiplier : 1.0;
            const double delta = (dir / 120.0) * state->style.wheel_step * mult;
            const double current = state->value.IsValid() ? state->value.Get() : state->style.initial_value;
            const double target = ClampAndQuantize(state->style, current + delta);
            if(state->set_value_fn) {
              state->set_value_fn(target);
            } else if(state->value.IsValid()) {
              state->value = target;
            }
            return true;
          });
    });
  });
}

} // namespace

double RadialDialController::Value() const {
  if(!state_ || !state_->value.IsValid()) return 0.0;
  return state_->value.Get();
}

float RadialDialController::Normalized() const {
  if(!state_ || !state_->normalized.IsValid()) return 0.0F;
  return state_->normalized.Get();
}

double RadialDialController::AngleDegrees() const {
  if(!state_ || !state_->angle_degrees.IsValid()) return 135.0;
  return state_->angle_degrees.Get();
}

bool RadialDialController::IsDragging() const {
  if(!state_ || !state_->dragging.IsValid()) return false;
  return state_->dragging.Get();
}

void RadialDialController::SetValue(double value) const {
  internal::RequireFinite(value, "HuxerUI Interaction: RadialDial SetValue target must be finite");
  if(!state_) return;
  if(state_->set_value_fn) {
    state_->set_value_fn(value);
  } else if(state_->value.IsValid()) {
    const double clamped = ClampAndQuantize(state_->style, value);
    const DialTelemetry tel = ComputeTelemetry(state_->style, clamped);
    state_->value = clamped;
    if(state_->normalized.IsValid()) state_->normalized = tel.normalized;
    if(state_->angle_degrees.IsValid()) state_->angle_degrees = tel.angle_degrees;
    if(state_->turns.IsValid()) state_->turns = tel.turns;
  }
}

void RadialDialController::Nudge(double delta) const {
  internal::RequireFinite(delta, "HuxerUI Interaction: RadialDial Nudge delta must be finite");
  if(!state_) return;
  SetValue(Value() + delta);
}

void RadialDialController::Reset() const {
  if(!state_) return;
  SetValue(state_->style.default_value.value_or(state_->style.initial_value));
}

RadialDialController UseRadialDialController(double initial_value) {
  internal::RequireFinite(initial_value,
                          "HuxerUI Interaction: UseRadialDialController initial value must be finite");
  auto value = UseState(initial_value);
  auto norm = UseState(0.0F);
  auto angle = UseState(135.0);
  auto turns = UseState(0.0);
  auto dragging = UseState(false);
  auto fine = UseState(false);
  auto hovered = UseState(false);
  auto focused = UseState(false);
  auto holder = UseState(std::make_shared<detail::RadialDialControllerState>());
  auto shared = holder.Get();
  shared->value = value;
  shared->normalized = norm;
  shared->angle_degrees = angle;
  shared->turns = turns;
  shared->dragging = dragging;
  shared->fine_precision = fine;
  shared->hovered = hovered;
  shared->focused = focused;
  shared->has_explicit_controller_initial = (initial_value != 0.0);
  return RadialDialController(std::move(shared));
}

View RadialDial(std::function<View(RadialDialSnapshot)> builder, RadialDialStyle style,
                std::function<void(double value)> on_change, std::function<void(double value)> on_commit) {
  if(!builder) {
    throw std::invalid_argument("HuxerUI Interaction: RadialDial requires a view builder");
  }
  auto external = style.controller.state_;
  return BuildRadialDialImpl(std::move(builder), std::move(style), std::move(on_change), std::move(on_commit),
                             std::move(external));
}

View RadialDial(RadialDialStyle style, std::function<void(double value)> on_change,
                std::function<void(double value)> on_commit) {
  auto external = style.controller.state_;
  return BuildRadialDialImpl({}, std::move(style), std::move(on_change), std::move(on_commit), std::move(external));
}

} // namespace huxerui::interaction
