#include <huxerui/interaction/visual/sharedmorph.h>

#include "../internal/validate.h"

#include <huxerui/state.h>

#include <stdexcept>
#include <utility>
#include <variant>

namespace huxerui::interaction {

namespace detail {
struct SharedMorphState {
  SharedMorphState(SharedTransitionHandle handle, AnimationSpec motion)
      : handle(std::move(handle)), default_motion(std::move(motion)) {}

  SharedTransitionHandle handle;
  AnimationSpec default_motion;
};
} // namespace detail

namespace {

void ValidateOneShotMotion(const AnimationSpec& spec) {
  if(const auto* tween = std::get_if<TweenSpec>(&spec)) {
    internal::RequireNonNegative(tween->duration,
                                 "HuxerUI Interaction: SharedMorph tween duration must be finite and non-negative");
  } else if(const auto* spring = std::get_if<SpringSpec>(&spec)) {
    internal::RequirePositive(spring->stiffness,
                              "HuxerUI Interaction: SharedMorph spring stiffness must be finite and positive");
    internal::RequirePositive(spring->damping_ratio,
                              "HuxerUI Interaction: SharedMorph spring damping ratio must be finite and positive");
  }
}

} // namespace

SharedTransitionScope SharedMorphHandle::Scope() const {
  if(!state_) {
    throw std::logic_error("HuxerUI Interaction: SharedMorphHandle is unbound");
  }
  return state_->handle.Scope();
}

void SharedMorphHandle::Run(std::function<void()> mutation) const {
  if(!state_) {
    throw std::logic_error("HuxerUI Interaction: SharedMorphHandle is unbound");
  }
  Run(state_->default_motion, std::move(mutation));
}

void SharedMorphHandle::Run(AnimationSpec motion, std::function<void()> mutation) const {
  if(!mutation) {
    throw std::invalid_argument("HuxerUI Interaction: SharedMorph requires a mutation");
  }
  ValidateOneShotMotion(motion);
  if(!state_) {
    throw std::logic_error("HuxerUI Interaction: SharedMorphHandle is unbound");
  }
  state_->handle.Run(std::move(motion), std::move(mutation));
}

SharedMorphHandle UseSharedMorph(SharedMorphStyle style) {
  ValidateOneShotMotion(style.motion);
  auto transition = UseSharedTransition();
  auto holder = UseState(std::make_shared<detail::SharedMorphState>(std::move(transition), style.motion));
  auto shared = holder.Get();
  shared->default_motion = std::move(style.motion);
  return SharedMorphHandle(std::move(shared));
}

ViewAdaptor MorphScope(SharedMorphHandle handle) {
  SharedTransitionScope scope = handle.Scope();
  return ViewAdaptor([scope = std::move(scope)](View content) { return std::move(content).With(scope); });
}

ViewAdaptor MorphElement(std::string key) {
  if(key.empty()) {
    throw std::invalid_argument("HuxerUI Interaction: MorphElement requires a non-empty key");
  }
  return ViewAdaptor(
      [key = std::move(key)](View content) { return std::move(content).With(SharedElement(key)); });
}

ViewAdaptor MorphBounds(std::string key) {
  if(key.empty()) {
    throw std::invalid_argument("HuxerUI Interaction: MorphBounds requires a non-empty key");
  }
  return ViewAdaptor(
      [key = std::move(key)](View content) { return std::move(content).With(SharedBounds(key)); });
}

} // namespace huxerui::interaction
