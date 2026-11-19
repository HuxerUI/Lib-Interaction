#include <huxerui/interaction/press/holdrepeat.h>

#include "../internal/validate.h"

#include <huxerui/layout.h>
#include <huxerui/lifecycle.h>
#include <huxerui/modifier.h>
#include <huxerui/state.h>
#include <huxerui/task.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <variant>

namespace huxerui::interaction {

namespace detail {

struct HoldRepeatControllerState {
  State<bool> holding;
  State<bool> repeating;
  State<std::size_t> trigger_count;
  std::function<void()> cancel_fn;
};

} // namespace detail

namespace {

constexpr double kDefaultAcceleratingMinIntervalSeconds = 0.03;

double ResolveMinimumIntervalSeconds(const HoldRepeatStyle& style) noexcept {
  if(style.minimum_interval.has_value()) {
    return style.minimum_interval->count();
  }
  return std::min(style.repeat_interval.count(), kDefaultAcceleratingMinIntervalSeconds);
}

void ValidateStyle(const HoldRepeatStyle& style) {
  internal::RequirePositiveDuration(style.initial_delay,
                                      "HuxerUI Interaction: HoldRepeat initial delay must be finite and positive");
  internal::RequirePositiveDuration(style.repeat_interval,
                                      "HuxerUI Interaction: HoldRepeat repeat interval must be finite and positive");
  internal::RequireOptionalPositiveDuration(
      style.minimum_interval, "HuxerUI Interaction: HoldRepeat minimum interval must be finite and positive");
  if(style.minimum_interval.has_value() && style.minimum_interval->count() > style.repeat_interval.count()) {
    throw std::invalid_argument(
        "HuxerUI Interaction: HoldRepeat minimum interval must not exceed repeat interval");
  }
  internal::RequirePositiveUnitInterval(style.acceleration,
                                        "HuxerUI Interaction: HoldRepeat acceleration must be finite and in (0, 1]");
  internal::RequirePositive(style.hovered_scale,
                            "HuxerUI Interaction: HoldRepeat hovered scale must be finite and positive");
  internal::RequirePositive(style.pressed_scale,
                            "HuxerUI Interaction: HoldRepeat pressed scale must be finite and positive");
  internal::RequireFinite(style.origin.x, "HuxerUI Interaction: HoldRepeat origin must be finite");
  internal::RequireFinite(style.origin.y, "HuxerUI Interaction: HoldRepeat origin must be finite");
  if(const auto* tween = std::get_if<TweenSpec>(&style.animation)) {
    internal::RequireNonNegative(tween->duration,
                                 "HuxerUI Interaction: HoldRepeat animation duration must be finite and non-negative");
  } else if(const auto* spring = std::get_if<SpringSpec>(&style.animation)) {
    internal::RequirePositive(spring->stiffness,
                              "HuxerUI Interaction: HoldRepeat animation spring stiffness must be finite and positive");
    internal::RequirePositive(
        spring->damping_ratio,
        "HuxerUI Interaction: HoldRepeat animation spring damping ratio must be finite and positive");
  }
}

enum class HoldSource { None, Pointer, Keyboard };

struct HoldRepeatRuntime final : std::enable_shared_from_this<HoldRepeatRuntime> {
  State<bool> hovered;
  State<bool> holding;
  State<bool> repeating;
  State<bool> focused;
  State<std::size_t> trigger_count;
  TaskScope tasks;
  std::shared_ptr<bool> alive;

  HoldRepeatStyle style;
  std::function<void(std::size_t)> on_trigger;
  std::function<void(bool)> on_holding_changed;
  std::shared_ptr<detail::HoldRepeatControllerState> controller;

  bool enabled = true;
  bool local_holding = false;
  bool local_repeating = false;
  std::size_t local_count = 0;
  HoldSource source = HoldSource::None;
  std::optional<std::int64_t> active_pointer;
  std::optional<Key> active_key;
  std::uint64_t generation = 0;
  TaskHandle active_task;

  void SyncStates() {
    if(holding.IsValid() && holding.Get() != local_holding) {
      holding = local_holding;
    }
    if(repeating.IsValid() && repeating.Get() != local_repeating) {
      repeating = local_repeating;
    }
    if(trigger_count.IsValid() && trigger_count.Get() != local_count) {
      trigger_count = local_count;
    }
    if(controller) {
      if(controller->holding.IsValid() && controller->holding.Get() != local_holding) {
        controller->holding = local_holding;
      }
      if(controller->repeating.IsValid() && controller->repeating.Get() != local_repeating) {
        controller->repeating = local_repeating;
      }
      if(controller->trigger_count.IsValid() && controller->trigger_count.Get() != local_count) {
        controller->trigger_count = local_count;
      }
    }
  }

  void ResetTracking() noexcept {
    active_task.Cancel();
    active_task = {};
    ++generation;
    source = HoldSource::None;
    active_pointer.reset();
    active_key.reset();
    local_holding = false;
    local_repeating = false;
    local_count = 0;
  }

  void StartHold(HoldSource next_source, std::optional<std::int64_t> pointer_id, std::optional<Key> key) {
    if(!alive || !*alive || !enabled || source != HoldSource::None) {
      return;
    }
    active_task.Cancel();
    active_task = {};
    const std::uint64_t gen = ++generation;
    source = next_source;
    active_pointer = pointer_id;
    active_key = key;
    local_holding = true;
    local_repeating = false;
    local_count = style.fire_on_press ? 1U : 0U;
    SyncStates();

    if(on_holding_changed) {
      auto cb = on_holding_changed;
      cb(true);
    }
    if(!*alive || generation != gen || !enabled || source == HoldSource::None) {
      return;
    }

    if(style.fire_on_press && on_trigger) {
      auto cb = on_trigger;
      cb(1U);
    }
    if(!*alive || generation != gen || !enabled || source == HoldSource::None) {
      return;
    }

    std::weak_ptr<HoldRepeatRuntime> weak_self = shared_from_this();
    const std::shared_ptr<bool> token_alive = alive;
    active_task = tasks.Launch([weak_self, token_alive, gen]() -> Task<void> {
      std::chrono::duration<double> initial_delay{};
      {
        std::shared_ptr<HoldRepeatRuntime> initial_self = weak_self.lock();
        if(!initial_self || !*token_alive || initial_self->generation != gen || !initial_self->enabled ||
           initial_self->source == HoldSource::None) {
          co_return;
        }
        initial_delay = initial_self->style.initial_delay;
      }

      co_await Delay(initial_delay);

      double interval = 0.0;
      bool interval_initialized = false;
      while(true) {
        std::chrono::duration<double> next_wait{};
        {
          std::shared_ptr<HoldRepeatRuntime> self = weak_self.lock();
          if(!self || !*token_alive || self->generation != gen || !self->enabled ||
             self->source == HoldSource::None) {
            co_return;
          }
          if(!interval_initialized) {
            interval = self->style.repeat_interval.count();
            interval_initialized = true;
          }
          self->local_repeating = true;
          self->local_count += 1U;
          const std::size_t step_count = self->local_count;
          self->SyncStates();
          if(self->on_trigger) {
            auto cb = self->on_trigger;
            cb(step_count);
          }
          if(!*token_alive || self->generation != gen || !self->enabled || self->source == HoldSource::None) {
            co_return;
          }
          const double min_interval = ResolveMinimumIntervalSeconds(self->style);
          const double max_interval = self->style.repeat_interval.count();
          const double wait_seconds = std::clamp(interval, min_interval, max_interval);
          interval = std::clamp(wait_seconds * static_cast<double>(self->style.acceleration), min_interval,
                                max_interval);
          next_wait = std::chrono::duration<double>(wait_seconds);
        }

        co_await Delay(next_wait);
      }
    });
  }

  void StopHold(bool notify_callback) {
    const bool was_holding = local_holding;
    ResetTracking();
    SyncStates();
    if(notify_callback && was_holding && on_holding_changed) {
      auto cb = on_holding_changed;
      cb(false);
    }
  }

  void Teardown() noexcept {
    ResetTracking();
    if(controller) {
      controller->cancel_fn = {};
    }
  }
};

struct HoldRepeatModifier {
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  std::shared_ptr<HoldRepeatRuntime> runtime;

  bool operator==(const HoldRepeatModifier&) const = default;
};

class HoldRepeatModifier::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const HoldRepeatModifier& value) { Update(node, value); }

  ~Extension() override {
    if(runtime_) {
      runtime_->active_task.Cancel();
    }
  }

  void Update(ViewNode&, const HoldRepeatModifier& value) { runtime_ = value.runtime; }

  [[nodiscard]] bool HitTest(ViewNode& node, Point position) const override {
    return node.IsEnabled() && node.Bounds().Contains(position);
  }

  [[nodiscard]] bool HoverHitTest(ViewNode& node, Point position) const override {
    return node.IsEnabled() && node.Bounds().Contains(position);
  }

  void OnHover(ViewNode&, const HoverEvent& event) override {
    if(!runtime_) return;
    const bool is_hovered = event.type != HoverEventType::Leave;
    if(runtime_->hovered.IsValid() && runtime_->hovered.Get() != is_hovered) {
      runtime_->hovered = is_hovered;
    }
  }

  void OnFocusChanged(ViewNode&, bool focused, bool) override {
    if(!runtime_) return;
    if(runtime_->focused.IsValid() && runtime_->focused.Get() != focused) {
      runtime_->focused = focused;
    }
    if(!focused && runtime_->source == HoldSource::Keyboard) {
      runtime_->StopHold(true);
    }
  }

  void OnInteraction(ViewNode&, const InteractionState& state, const std::optional<InteractionEvent>&) override {
    if(!runtime_) return;
    runtime_->enabled = state.enabled;
    if(!state.enabled && runtime_->local_holding) {
      runtime_->ResetTracking();
      std::weak_ptr<HoldRepeatRuntime> weak_rt = runtime_;
      const std::shared_ptr<bool> token_alive = runtime_->alive;
      if(token_alive && *token_alive) {
        runtime_->tasks.Post([weak_rt, token_alive]() {
          if(!*token_alive) return;
          if(auto rt = weak_rt.lock()) {
            if(!rt->local_holding) {
              rt->SyncStates();
            }
            if(rt->on_holding_changed) {
              auto cb = rt->on_holding_changed;
              cb(false);
            }
          }
        });
      }
    }
  }

  PointerResult OnPointer(ViewNode& node, const PointerEvent& event) override {
    if(!runtime_ || !node.IsEnabled()) {
      return PointerResult::Ignored;
    }
    if(event.type == PointerEventType::Down) {
      if(event.changed_button != PointerButton::Primary || runtime_->source != HoldSource::None) {
        return PointerResult::Ignored;
      }
      runtime_->StartHold(HoldSource::Pointer, event.pointer_id, std::nullopt);
      return runtime_->source == HoldSource::Pointer ? PointerResult::Capture : PointerResult::Handled;
    }
    if(runtime_->source != HoldSource::Pointer || !runtime_->active_pointer.has_value() ||
       *runtime_->active_pointer != event.pointer_id) {
      return PointerResult::Ignored;
    }
    if(event.type == PointerEventType::Move) {
      if(runtime_->style.cancel_on_leave && !node.Bounds().Contains(event.position)) {
        runtime_->StopHold(true);
      }
      return PointerResult::Handled;
    }
    if(event.type == PointerEventType::Up) {
      if(event.changed_button == PointerButton::Primary ||
         (event.pressed_buttons & PointerButton::Primary) == PointerButton::None) {
        runtime_->StopHold(true);
      }
      return PointerResult::Handled;
    }
    if(event.type == PointerEventType::Cancel) {
      runtime_->StopHold(true);
      return PointerResult::Handled;
    }
    return PointerResult::Handled;
  }

  bool OnKey(ViewNode& node, const KeyEvent& event) override {
    if(!runtime_ || !runtime_->style.keyboard_focusable || !node.IsEnabled()) {
      return false;
    }
    if(event.key != Key::Space && event.key != Key::Enter) {
      return false;
    }
    if(event.type == KeyEventType::Down) {
      if(event.modifiers.shift || event.modifiers.control || event.modifiers.alt || event.modifiers.meta) {
        return false;
      }
      if(event.repeat) {
        return runtime_->source == HoldSource::Keyboard && runtime_->active_key == event.key;
      }
      if(runtime_->source != HoldSource::None) {
        return false;
      }
      runtime_->StartHold(HoldSource::Keyboard, std::nullopt, event.key);
      return true;
    }
    if(event.type == KeyEventType::Up && runtime_->source == HoldSource::Keyboard &&
       runtime_->active_key == event.key) {
      runtime_->StopHold(true);
      return true;
    }
    return false;
  }

private:
  std::shared_ptr<HoldRepeatRuntime> runtime_;
};

const huxerui::detail::ModifierDescriptor& HoldRepeatModifier::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<HoldRepeatModifier, HoldRepeatModifier::Extension>();
}

} // namespace

bool HoldRepeatController::IsHolding() const {
  if(!state_ || !state_->holding.IsValid()) return false;
  return state_->holding.Get();
}

bool HoldRepeatController::IsRepeating() const {
  if(!state_ || !state_->repeating.IsValid()) return false;
  return state_->repeating.Get();
}

std::size_t HoldRepeatController::TriggerCount() const {
  if(!state_ || !state_->trigger_count.IsValid()) return 0U;
  return state_->trigger_count.Get();
}

void HoldRepeatController::Cancel() const {
  if(state_ && state_->cancel_fn) {
    auto fn = state_->cancel_fn;
    fn();
  }
}

HoldRepeatController UseHoldRepeatController() {
  auto holding = UseState(false);
  auto repeating = UseState(false);
  auto trigger_count = UseState<std::size_t>(0U);
  auto holder = UseState(std::make_shared<detail::HoldRepeatControllerState>());
  auto shared = holder.Get();
  shared->holding = holding;
  shared->repeating = repeating;
  shared->trigger_count = trigger_count;
  return HoldRepeatController(std::move(shared));
}

namespace detail {

View HoldRepeatImpl(std::function<View(HoldRepeatSnapshot)> builder,
                    std::function<void(std::size_t)> on_trigger, HoldRepeatStyle style,
                    std::function<void(bool)> on_holding_changed) {
  if(!builder) {
    throw std::invalid_argument("HuxerUI Interaction: HoldRepeat requires a view builder");
  }
  ValidateStyle(style);
  const bool actionable = static_cast<bool>(on_trigger) || static_cast<bool>(on_holding_changed);

  return Scope([builder = std::move(builder), on_trigger = std::move(on_trigger), style = std::move(style),
                on_holding_changed = std::move(on_holding_changed), actionable]() -> View {
    auto hovered = UseState(false);
    auto holding = UseState(false);
    auto repeating = UseState(false);
    auto focused = UseState(false);
    auto trigger_count = UseState<std::size_t>(0U);
    TaskScope tasks = UseTaskScope();
    auto alive = UseState(std::make_shared<bool>(false)).Get();
    auto runtime = UseState(std::make_shared<HoldRepeatRuntime>()).Get();

    if(runtime->controller && runtime->controller != style.controller.state_) {
      runtime->controller->cancel_fn = {};
    }
    runtime->hovered = hovered;
    runtime->holding = holding;
    runtime->repeating = repeating;
    runtime->focused = focused;
    runtime->trigger_count = trigger_count;
    runtime->tasks = tasks;
    runtime->alive = alive;
    runtime->style = style;
    runtime->on_trigger = on_trigger;
    runtime->on_holding_changed = on_holding_changed;
    runtime->controller = style.controller.state_;
    if(runtime->controller) {
      std::weak_ptr<HoldRepeatRuntime> weak_rt = runtime;
      runtime->controller->cancel_fn = [weak_rt, alive]() {
        if(!*alive) return;
        if(auto rt = weak_rt.lock()) {
          rt->StopHold(true);
        }
      };
    }

    std::weak_ptr<HoldRepeatRuntime> weak_rt = runtime;
    Lifecycle(
        [weak_rt, alive]() {
          *alive = true;
          return [weak_rt, alive]() {
            *alive = false;
            if(auto rt = weak_rt.lock()) {
              rt->Teardown();
            }
          };
        },
        runtime);

    const HoldRepeatSnapshot snapshot{
        .hovered = hovered.Get(),
        .holding = holding.Get(),
        .focused = focused.Get(),
        .repeating = repeating.Get(),
        .trigger_count = trigger_count.Get(),
    };

    View surface = builder(snapshot);
    surface = std::move(surface).With(HoldRepeatModifier{.runtime = runtime});
    if(actionable && style.keyboard_focusable) {
      surface = std::move(surface).With(Focusable{true});
    }
    if(actionable && style.role.has_value()) {
      surface = std::move(surface).With(Semantics{.role = *style.role});
    }
    if(style.cursor != PointerCursorKind::Default) {
      surface = std::move(surface).With(PointerCursor{style.cursor});
    }
    return surface;
  });
}

View HoldRepeatWrapImpl(View content, std::function<void(std::size_t)> on_trigger, HoldRepeatStyle style,
                        std::function<void(bool)> on_holding_changed) {
  ValidateStyle(style);
  return HoldRepeatImpl(
      [content = std::move(content), style](HoldRepeatSnapshot snapshot) -> View {
        const float target_scale = snapshot.holding ? style.pressed_scale
                                                    : (snapshot.hovered ? style.hovered_scale : 1.0F);
        View inner = content;
        inner = std::move(inner).With(Scale(AnimateTo(target_scale, style.animation), style.origin));
        return Stack{std::move(inner)};
      },
      std::move(on_trigger), std::move(style), std::move(on_holding_changed));
}

} // namespace detail
} // namespace huxerui::interaction
