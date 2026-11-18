#include <huxerui/interaction/visual/shake.h>

#include "../internal/validate.h"

#include <huxerui/state.h>
#include <huxerui/task.h>

#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {

namespace detail {
struct ShakeTriggerState {
  State<Point> offset;
  State<std::uint64_t> run;
  TaskScope tasks;
  ShakeStyle style;
};
} // namespace detail

namespace {

void ValidateStyle(const ShakeStyle& style) {
  internal::RequirePositive(style.amplitude, "HuxerUI Interaction: Shake amplitude must be finite and positive");
  if(style.wobbles < 1) {
    throw std::invalid_argument("HuxerUI Interaction: Shake wobbles must be at least one");
  }
  internal::RequirePositive(style.duration, "HuxerUI Interaction: Shake duration must be finite and positive");
}

} // namespace

ShakeTrigger UseShake(ShakeStyle style) {
  ValidateStyle(style);
  auto offset = UseState(Point{});
  auto run = UseState(std::uint64_t{0});
  auto tasks = UseTaskScope();
  auto holder = UseState(std::make_shared<detail::ShakeTriggerState>());
  auto shared = holder.Get();
  shared->offset = offset;
  shared->run = run;
  shared->tasks = tasks;
  shared->style = style;
  return ShakeTrigger(std::move(shared));
}

void ShakeTrigger::Trigger() const {
  if(!state_ || !state_->offset.IsValid() || !state_->run.IsValid()) return;
  const std::uint64_t id = state_->run.Get() + 1;
  state_->run = id;
  const ShakeStyle style = state_->style;
  const Point direction =
      style.axis == Axis::Vertical ? Point{0.0F, style.amplitude} : Point{style.amplitude, 0.0F};
  state_->offset = direction;
  state_->tasks.Launch([offset = state_->offset, run = state_->run, id, style, direction]() -> Task<void> {
    for(int wobble = 0; wobble < style.wobbles; ++wobble) {
      co_await Delay(std::chrono::duration<double>(style.duration));
      if(run.Get() != id) co_return;
      offset = Point{};
      if(wobble + 1 < style.wobbles) {
        co_await Delay(std::chrono::duration<double>(style.duration));
        if(run.Get() != id) co_return;
        const float decay = static_cast<float>(wobble + 1) / static_cast<float>(style.wobbles);
        offset = Point{direction.x * (1.0F - decay), direction.y * (1.0F - decay)};
      }
    }
  });
}

View Shaken(View content, const ShakeTrigger& shake) {
  if(!shake) return content;
  return Scope([content = std::move(content), state = shake.state_]() -> View {
    const TweenSpec leg{.duration = state->style.duration, .easing = state->style.easing};
    View shaken = content;
    return std::move(shaken).With(Offset(AnimateTo(state->offset.Get(), leg)));
  });
}

} // namespace huxerui::interaction
