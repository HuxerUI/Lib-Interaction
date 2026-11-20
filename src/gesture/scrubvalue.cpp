#include <huxerui/interaction/gesture/scrubvalue.h>

#include "../internal/validate.h"

#include <huxerui/layout.h>
#include <huxerui/lifecycle.h>
#include <huxerui/modifier.h>
#include <huxerui/semantics.h>
#include <huxerui/state.h>
#include <huxerui/task.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {

namespace detail {

struct ScrubValueControllerState {
  State<double> value;
  State<double> drag_delta;
  State<bool> scrubbing;
  State<bool> hovered;
  State<bool> focused;
  State<ScrubPrecision> precision;
  ScrubValueStyle style;
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
constexpr float kDoubleClickMaxDistance = 10.0F;
constexpr auto kDoubleClickInterval = std::chrono::duration<double>(0.35);

void ValidateStyle(const ScrubValueStyle& style) {
  if(std::isnan(style.min_value) || style.min_value == std::numeric_limits<double>::infinity()) {
    throw std::invalid_argument(
        "HuxerUI Interaction: ScrubValue min_value must not be NaN or +infinity");
  }
  if(std::isnan(style.max_value) || style.max_value == -std::numeric_limits<double>::infinity()) {
    throw std::invalid_argument(
        "HuxerUI Interaction: ScrubValue max_value must not be NaN or -infinity");
  }
  if(style.min_value > style.max_value) {
    throw std::invalid_argument("HuxerUI Interaction: ScrubValue min_value must not exceed max_value");
  }
  internal::RequireFinite(style.initial_value,
                          "HuxerUI Interaction: ScrubValue initial_value must be finite");
  if(style.initial_value < style.min_value || style.initial_value > style.max_value) {
    throw std::invalid_argument(
        "HuxerUI Interaction: ScrubValue initial_value must lie within [min_value, max_value]");
  }
  if(style.default_value.has_value()) {
    internal::RequireFinite(*style.default_value,
                            "HuxerUI Interaction: ScrubValue default_value must be finite");
    if(*style.default_value < style.min_value || *style.default_value > style.max_value) {
      throw std::invalid_argument(
          "HuxerUI Interaction: ScrubValue default_value must lie within [min_value, max_value]");
    }
  }
  internal::RequirePositive(style.sensitivity,
                            "HuxerUI Interaction: ScrubValue sensitivity must be finite and positive");
  internal::RequirePositive(style.fine_multiplier,
                            "HuxerUI Interaction: ScrubValue fine_multiplier must be finite and positive");
  internal::RequirePositive(style.coarse_multiplier,
                            "HuxerUI Interaction: ScrubValue coarse_multiplier must be finite and positive");
  if(style.step.has_value()) {
    internal::RequirePositive(*style.step,
                              "HuxerUI Interaction: ScrubValue step must be finite and positive");
  }
  internal::RequirePositive(style.keyboard_step,
                            "HuxerUI Interaction: ScrubValue keyboard_step must be finite and positive");
  internal::RequireNonNegative(style.drag_slop,
                               "HuxerUI Interaction: ScrubValue drag_slop must be finite and non-negative");
}

double ClampAndQuantize(const ScrubValueStyle& style, double raw) noexcept {
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

ScrubPrecision PrecisionFromModifiers(const KeyModifiers& mods) noexcept {
  if(mods.shift) return ScrubPrecision::Fine;
  if(mods.alt) return ScrubPrecision::Coarse;
  return ScrubPrecision::Normal;
}

double MultiplierForPrecision(const ScrubValueStyle& style, ScrubPrecision precision) noexcept {
  switch(precision) {
  case ScrubPrecision::Fine:
    return style.fine_multiplier;
  case ScrubPrecision::Coarse:
    return style.coarse_multiplier;
  case ScrubPrecision::Normal:
    return 1.0;
  }
  return 1.0;
}

void ScheduleSync(detail::ScrubValueControllerState& state) {
  if(state.sync_scheduled || !state.alive || !*state.alive || !state.post_sync) return;
  state.sync_scheduled = true;
  state.post_sync();
}

class ScrubValuePassThroughLayout final : public Layout<ScrubValuePassThroughLayout> {
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

struct ScrubValueModifier {
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  std::shared_ptr<detail::ScrubValueControllerState> state;
  TaskScope tasks{};
  double value = 0.0;
  double min_value = -std::numeric_limits<double>::infinity();
  double max_value = std::numeric_limits<double>::infinity();
  double keyboard_step = 1.0;
  std::optional<double> step{};

  bool operator==(const ScrubValueModifier& other) const noexcept {
    return state == other.state && value == other.value && min_value == other.min_value
           && max_value == other.max_value && keyboard_step == other.keyboard_step && step == other.step;
  }
};

class ScrubValueModifier::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const ScrubValueModifier& value) { Update(node, value); }

  ~Extension() override {
    ++click_token_;
    if(state_) {
      state_->set_value_fn = {};
    }
  }

  void Update(ViewNode&, const ScrubValueModifier& value) {
    const bool state_changed = state_ != value.state;
    if(state_ && state_changed) {
      state_->set_value_fn = {};
      pointer_id_.reset();
      scrub_active_ = false;
      ++click_token_;
      pending_click_ = false;
    }
    state_ = value.state;
    tasks_ = value.tasks;
    if(state_) {
      state_->set_value_fn = [this](double target) { ProgrammaticSetValue(target); };
      if(state_changed) {
        raw_value_ = CurrentValue();
        state_->needs_sync = true;
      }
    }
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
  }

  void OnFocusChanged(ViewNode&, bool focused, bool) override {
    if(!state_) return;
    if(state_->focused.IsValid() && state_->focused.Get() != focused) {
      state_->focused = focused;
    }
  }

  void OnInteraction(ViewNode&, const InteractionState& state, const std::optional<InteractionEvent>&) override {
    if(!state_) return;
    state_->enabled = state.enabled;
    if(!state.enabled && pointer_id_.has_value()) {
      pointer_id_.reset();
      scrub_active_ = false;
      ++click_token_;
      pending_click_ = false;
      state_->needs_sync = true;
      ScheduleSync(*state_);
    }
  }

  PointerResult OnPointer(ViewNode& node, const PointerEvent& event) override {
    if(!state_ || !node.IsEnabled() || !state_->enabled) {
      if(pointer_id_.has_value()) {
        CancelActiveScrub(false);
      }
      return PointerResult::Ignored;
    }

    if(event.type == PointerEventType::Down) {
      if(event.changed_button != PointerButton::Primary || pointer_id_.has_value()) {
        return PointerResult::Ignored;
      }
      pointer_id_ = event.pointer_id;
      down_position_ = event.position;
      last_position_ = event.position;
      drag_start_value_ = ClampAndQuantize(state_->style, CurrentValue());
      raw_value_ = drag_start_value_;
      scrub_active_ = state_->style.drag_slop <= 0.0F;
      if(scrub_active_) {
        ++click_token_;
        pending_click_ = false;
        SetScrubbingStates(true, PrecisionFromModifiers(event.modifiers), 0.0);
      }
      return PointerResult::Capture;
    }

    if(!pointer_id_.has_value() || *pointer_id_ != event.pointer_id) {
      return PointerResult::Ignored;
    }

    if(event.type == PointerEventType::Move) {
      Point delta{};
      if(!scrub_active_) {
        const float dist = std::hypot(event.position.x - down_position_.x, event.position.y - down_position_.y);
        if(dist < state_->style.drag_slop) {
          return PointerResult::Handled;
        }
        scrub_active_ = true;
        ++click_token_;
        pending_click_ = false;
        delta = event.position - down_position_;
      } else {
        delta = event.position - last_position_;
      }
      last_position_ = event.position;

      const ScrubPrecision tier = PrecisionFromModifiers(event.modifiers);
      const double mult = MultiplierForPrecision(state_->style, tier);
      const double signed_px = state_->style.axis == Axis::Horizontal ? static_cast<double>(delta.x)
                                                                      : -static_cast<double>(delta.y);
      const double directional_px = state_->style.invert_axis ? -signed_px : signed_px;
      raw_value_ = std::clamp(raw_value_ + directional_px * state_->style.sensitivity * mult,
                              state_->style.min_value, state_->style.max_value);
      const double next = ClampAndQuantize(state_->style, raw_value_);
      ApplyValue(next, true, false);
      SetScrubbingStates(true, tier, next - drag_start_value_);
      return PointerResult::Handled;
    }

    if(event.type == PointerEventType::Up) {
      const bool was_scrubbing = scrub_active_;
      pointer_id_.reset();
      scrub_active_ = false;
      SetScrubbingStates(false, ScrubPrecision::Normal, 0.0);
      if(was_scrubbing) {
        const double final_value = ClampAndQuantize(state_->style, CurrentValue());
        if(std::abs(final_value - drag_start_value_) > kValueEpsilon && state_->on_commit) {
          auto cb = state_->on_commit;
          cb(final_value);
        }
      } else if(state_->style.double_click_resets) {
        HandleCleanClick(event.position);
      }
      return PointerResult::Handled;
    }

    if(event.type == PointerEventType::Cancel) {
      CancelActiveScrub(true);
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
      CancelActiveScrub(true);
      return true;
    }
    if(pointer_id_.has_value() || event.modifiers.control || event.modifiers.meta) {
      return false;
    }

    const ScrubPrecision tier = PrecisionFromModifiers(event.modifiers);
    const double mult = MultiplierForPrecision(state_->style, tier);
    const double base = state_->style.step.value_or(state_->style.keyboard_step);
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
      return true;
    }
    if(action.kind == SemanticActionKind::Decrement) {
      const double next = ClampAndQuantize(state_->style, current - base);
      raw_value_ = next;
      ApplyValue(next, true, true);
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

private:
  [[nodiscard]] double CurrentValue() const {
    if(!state_) return 0.0;
    if(state_->value.IsValid()) return state_->value.Get();
    return state_->style.initial_value;
  }

  void SetScrubbingStates(bool scrubbing, ScrubPrecision tier, double delta) {
    if(!state_) return;
    if(state_->scrubbing.IsValid() && state_->scrubbing.Get() != scrubbing) {
      state_->scrubbing = scrubbing;
    }
    if(state_->precision.IsValid() && state_->precision.Get() != tier) {
      state_->precision = tier;
    }
    const double clean_delta = std::abs(delta) < kValueEpsilon ? 0.0 : delta;
    if(state_->drag_delta.IsValid() && std::abs(state_->drag_delta.Get() - clean_delta) > kValueEpsilon) {
      state_->drag_delta = clean_delta;
    }
  }

  void ApplyValue(double next, bool notify_change, bool notify_commit) {
    if(!state_ || !state_->value.IsValid()) return;
    const double prev = state_->value.Get();
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
  }

  void CancelActiveScrub(bool restore_value) {
    const bool was_scrubbing = scrub_active_;
    pointer_id_.reset();
    scrub_active_ = false;
    ++click_token_;
    pending_click_ = false;
    if(!state_) return;
    SetScrubbingStates(false, ScrubPrecision::Normal, 0.0);
    if(restore_value && was_scrubbing) {
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
    std::weak_ptr<detail::ScrubValueControllerState> weak_state = state_;
    tasks_.Launch([this, alive, weak_state, token]() -> Task<void> {
      co_await Delay(kDoubleClickInterval);
      if(!alive || !*alive || weak_state.expired() || click_token_ != token) {
        co_return;
      }
      pending_click_ = false;
    });
  }

  std::shared_ptr<detail::ScrubValueControllerState> state_;
  TaskScope tasks_{};
  std::optional<std::int64_t> pointer_id_{};
  Point down_position_{};
  Point last_position_{};
  double drag_start_value_ = 0.0;
  double raw_value_ = 0.0;
  bool scrub_active_ = false;

  bool pending_click_ = false;
  Point pending_click_pos_{};
  std::uint64_t click_token_ = 0;
};

const huxerui::detail::ModifierDescriptor& ScrubValueModifier::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<ScrubValueModifier, ScrubValueModifier::Extension>();
}

View BuildScrubValueImpl(std::function<View(ScrubValueSnapshot)> builder, View wrapped_content, ScrubValueStyle style,
                         std::function<void(double value)> on_change, std::function<void(double value)> on_commit,
                         std::shared_ptr<detail::ScrubValueControllerState> external_state) {
  ValidateStyle(style);
  const bool has_builder = static_cast<bool>(builder);

  return Scope([builder = std::move(builder), wrapped_content = std::move(wrapped_content), style = std::move(style),
                on_change = std::move(on_change), on_commit = std::move(on_commit),
                external_state = std::move(external_state), has_builder]() mutable -> View {
    TaskScope tasks = UseTaskScope();
    auto alive = UseState(std::make_shared<bool>(true)).Get();
    auto local_value = UseState(ClampAndQuantize(style, style.initial_value));
    auto local_delta = UseState(0.0);
    auto local_scrubbing = UseState(false);
    auto local_hovered = UseState(false);
    auto local_focused = UseState(false);
    auto local_precision = UseState(ScrubPrecision::Normal);
    auto local_holder = UseState(std::make_shared<detail::ScrubValueControllerState>());

    std::shared_ptr<detail::ScrubValueControllerState> state = external_state ? external_state : local_holder.Get();
    if(!external_state) {
      state->value = local_value;
      state->drag_delta = local_delta;
      state->scrubbing = local_scrubbing;
      state->hovered = local_hovered;
      state->focused = local_focused;
      state->precision = local_precision;
    }

    state->style = style;
    state->style.controller = {};
    state->on_change = on_change;
    state->on_commit = on_commit;
    state->alive = alive;
    *alive = true;

    std::weak_ptr<detail::ScrubValueControllerState> weak_state = state;
    state->post_sync = [tasks, weak_state, alive]() {
      tasks.Post([weak_state, alive]() {
        if(!*alive) return;
        auto locked = weak_state.lock();
        if(!locked) return;
        locked->sync_scheduled = false;

        if(!locked->enabled) {
          if(locked->scrubbing.IsValid() && locked->scrubbing.Get()) locked->scrubbing = false;
          if(locked->precision.IsValid() && locked->precision.Get() != ScrubPrecision::Normal) {
            locked->precision = ScrubPrecision::Normal;
          }
          if(locked->drag_delta.IsValid() && locked->drag_delta.Get() != 0.0) locked->drag_delta = 0.0;
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

    return Scope([builder, wrapped_content, state, tasks, has_builder]() -> View {
      const double raw = state->value.IsValid() ? state->value.Get() : state->style.initial_value;
      const double value = ClampAndQuantize(state->style, raw);
      const double drag_delta = state->drag_delta.IsValid() ? state->drag_delta.Get() : 0.0;
      const bool scrubbing = state->scrubbing.IsValid() && state->scrubbing.Get();
      const bool hovered = state->hovered.IsValid() && state->hovered.Get();
      const bool focused = state->focused.IsValid() && state->focused.Get();
      const ScrubPrecision tier = state->precision.IsValid() ? state->precision.Get() : ScrubPrecision::Normal;

      View child = has_builder ? builder(ScrubValueSnapshot{
                                     .value = value,
                                     .drag_delta = drag_delta,
                                     .scrubbing = scrubbing,
                                     .hovered = hovered,
                                     .focused = focused,
                                     .precision = tier,
                                 })
                               : wrapped_content;

      const PointerCursorKind cursor = state->style.cursor != PointerCursorKind::Default
                                           ? state->style.cursor
                                           : (state->style.axis == Axis::Horizontal ? PointerCursorKind::ResizeHorizontal
                                                                                    : PointerCursorKind::ResizeVertical);

      return ScrubValuePassThroughLayout(std::move(child))
          .With(Focusable{state->style.keyboard_focusable}, PointerCursor{cursor},
                ScrubValueModifier{
                    .state = state,
                    .tasks = tasks,
                    .value = value,
                    .min_value = state->style.min_value,
                    .max_value = state->style.max_value,
                    .keyboard_step = state->style.keyboard_step,
                    .step = state->style.step,
                });
    });
  });
}

} // namespace

double ScrubValueController::Value() const {
  if(!state_ || !state_->value.IsValid()) return 0.0;
  return state_->value.Get();
}

bool ScrubValueController::IsScrubbing() const {
  if(!state_ || !state_->scrubbing.IsValid()) return false;
  return state_->scrubbing.Get();
}

ScrubPrecision ScrubValueController::Precision() const {
  if(!state_ || !state_->precision.IsValid()) return ScrubPrecision::Normal;
  return state_->precision.Get();
}

void ScrubValueController::SetValue(double value) const {
  internal::RequireFinite(value, "HuxerUI Interaction: ScrubValue SetValue target must be finite");
  if(!state_) return;
  if(state_->set_value_fn) {
    state_->set_value_fn(value);
  } else if(state_->value.IsValid()) {
    state_->value = ClampAndQuantize(state_->style, value);
  }
}

void ScrubValueController::Nudge(double delta) const {
  internal::RequireFinite(delta, "HuxerUI Interaction: ScrubValue Nudge delta must be finite");
  if(!state_) return;
  SetValue(Value() + delta);
}

void ScrubValueController::Reset() const {
  if(!state_) return;
  SetValue(state_->style.default_value.value_or(state_->style.initial_value));
}

ScrubValueController UseScrubValueController(double initial_value) {
  internal::RequireFinite(initial_value,
                          "HuxerUI Interaction: UseScrubValueController initial value must be finite");
  auto value = UseState(initial_value);
  auto delta = UseState(0.0);
  auto scrubbing = UseState(false);
  auto hovered = UseState(false);
  auto focused = UseState(false);
  auto precision = UseState(ScrubPrecision::Normal);
  auto holder = UseState(std::make_shared<detail::ScrubValueControllerState>());
  auto shared = holder.Get();
  shared->value = value;
  shared->drag_delta = delta;
  shared->scrubbing = scrubbing;
  shared->hovered = hovered;
  shared->focused = focused;
  shared->precision = precision;
  shared->has_explicit_controller_initial = (initial_value != 0.0);
  return ScrubValueController(std::move(shared));
}

View ScrubValue(std::function<View(ScrubValueSnapshot)> builder, ScrubValueStyle style,
                std::function<void(double value)> on_change, std::function<void(double value)> on_commit) {
  if(!builder) {
    throw std::invalid_argument("HuxerUI Interaction: ScrubValue requires a view builder");
  }
  auto external = style.controller.state_;
  return BuildScrubValueImpl(std::move(builder), {}, std::move(style), std::move(on_change), std::move(on_commit),
                             std::move(external));
}

View ScrubValue(View content, ScrubValueStyle style, std::function<void(double value)> on_change,
                std::function<void(double value)> on_commit) {
  auto external = style.controller.state_;
  return BuildScrubValueImpl({}, std::move(content), std::move(style), std::move(on_change), std::move(on_commit),
                             std::move(external));
}

} // namespace huxerui::interaction
