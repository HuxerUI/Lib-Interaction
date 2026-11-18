#include <huxerui/interaction/visual/pop.h>

#include "../internal/validate.h"

#include <huxerui/state.h>
#include <huxerui/task.h>

#include <chrono>
#include <cstdint>
#include <utility>

namespace huxerui::interaction {

namespace detail {
struct PopTriggerState {
  State<float> scale;
  State<std::uint64_t> run;
  TaskScope tasks;
  PopStyle style;
};
} // namespace detail

namespace {

void ValidateStyle(const PopStyle& style) {
  internal::RequirePositive(style.amplitude, "HuxerUI Interaction: Pop amplitude must be finite and positive");
  internal::RequirePositive(style.duration, "HuxerUI Interaction: Pop duration must be finite and positive");
}

} // namespace

PopTrigger UsePop(PopStyle style) {
  ValidateStyle(style);
  auto scale = UseState(1.0F);
  auto run = UseState(std::uint64_t{0});
  auto tasks = UseTaskScope();
  auto holder = UseState(std::make_shared<detail::PopTriggerState>());
  auto shared = holder.Get();
  shared->scale = scale;
  shared->run = run;
  shared->tasks = tasks;
  shared->style = style;
  return PopTrigger(std::move(shared));
}

void PopTrigger::Trigger() const {
  if(!state_ || !state_->scale.IsValid() || !state_->run.IsValid()) return;
  const std::uint64_t id = state_->run.Get() + 1;
  state_->run = id;
  state_->scale = 1.0F + state_->style.amplitude;
  state_->tasks.Launch([scale = state_->scale, run = state_->run, id, style = state_->style]() -> Task<void> {
    co_await Delay(std::chrono::duration<double>(style.duration));
    if(run.Get() != id) co_return;
    scale = 1.0F;
  });
}

View Popped(View content, const PopTrigger& pop) {
  if(!pop) return content;
  return Scope([content = std::move(content), state = pop.state_]() -> View {
    const TweenSpec leg{.duration = state->style.duration, .easing = state->style.easing};
    View popped = content;
    return std::move(popped).With(Scale(AnimateTo(state->scale.Get(), leg), state->style.origin));
  });
}

} // namespace huxerui::interaction
