#include <huxerui/interaction/pop.h>

#include <chrono>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const PopStyle& style) {
  if(!std::isfinite(style.amplitude) || style.amplitude <= 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: Pop amplitude must be finite and positive");
  }
  if(!std::isfinite(style.duration) || style.duration <= 0.0) {
    throw std::invalid_argument("HuxerUI Interaction: Pop duration must be finite and positive");
  }
}

} // namespace

PopTrigger UsePop(PopStyle style) {
  ValidateStyle(style);
  auto scale = UseState(1.0F);
  auto run = UseState(std::uint64_t{0});
  auto tasks = UseTaskScope();
  return PopTrigger(std::move(scale), std::move(run), std::move(tasks), style);
}

void PopTrigger::Trigger() const {
  if(!*this) return;
  const std::uint64_t id = run_.Get() + 1;
  run_ = id;
  scale_ = 1.0F + style_.amplitude;
  tasks_.Launch([scale = scale_, run = run_, id, style = style_]() -> Task<void> {
    co_await Delay(std::chrono::duration<double>(style.duration));
    if(run.Get() != id) co_return;
    scale = 1.0F;
  });
}

View Popped(View content, const PopTrigger& pop) {
  if(!pop) return content;
  return Scope([content = std::move(content), pop]() -> View {
    const TweenSpec leg{.duration = pop.style_.duration, .easing = pop.style_.easing};
    View popped = content;
    return std::move(popped).With(Scale(AnimateTo(pop.scale_.Get(), leg), pop.style_.origin));
  });
}

} // namespace huxerui::interaction
