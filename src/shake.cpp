#include <huxerui/interaction/shake.h>

#include <chrono>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const ShakeStyle& style) {
  if(!std::isfinite(style.amplitude) || style.amplitude <= 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: Shake amplitude must be finite and positive");
  }
  if(style.wobbles < 1) {
    throw std::invalid_argument("HuxerUI Interaction: Shake wobbles must be at least one");
  }
  if(!std::isfinite(style.duration) || style.duration <= 0.0) {
    throw std::invalid_argument("HuxerUI Interaction: Shake duration must be finite and positive");
  }
}

} // namespace

ShakeTrigger UseShake(ShakeStyle style) {
  ValidateStyle(style);
  auto offset = UseState(Point{});
  auto run = UseState(std::uint64_t{0});
  auto tasks = UseTaskScope();
  return ShakeTrigger(std::move(offset), std::move(run), std::move(tasks), style);
}

void ShakeTrigger::Trigger() const {
  if(!*this) return;
  const std::uint64_t id = run_.Get() + 1;
  run_ = id;
  offset_ = Point{style_.amplitude, 0.0F};
  tasks_.Launch([offset = offset_, run = run_, id, style = style_]() -> Task<void> {
    for(int wobble = 0; wobble < style.wobbles; ++wobble) {
      co_await Delay(std::chrono::duration<double>(style.duration));
      if(run.Get() != id) co_return;
      offset = Point{};
      if(wobble + 1 < style.wobbles) {
        co_await Delay(std::chrono::duration<double>(style.duration));
        if(run.Get() != id) co_return;
        const float decay = static_cast<float>(wobble + 1) / static_cast<float>(style.wobbles);
        offset = Point{style.amplitude * (1.0F - decay), 0.0F};
      }
    }
  });
}

View Shaken(View content, const ShakeTrigger& shake) {
  if(!shake) return content;
  return Scope([content = std::move(content), shake]() -> View {
    const TweenSpec leg{.duration = shake.style_.duration, .easing = shake.style_.easing};
    View shaken = content;
    return std::move(shaken).With(Offset(AnimateTo(shake.offset_.Get(), leg)));
  });
}

} // namespace huxerui::interaction
