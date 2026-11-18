#include <huxerui/interaction/visual/revealontap.h>

#include <stdexcept>
#include <utility>

namespace huxerui::interaction {

RevealOnTapHandle::RevealOnTapHandle(SceneTransitionHandle handle, TransitionSpec transition)
    : state_(BoundState{.handle = std::move(handle), .transition = std::move(transition)}) {}

void RevealOnTapHandle::Run(std::function<void()> mutation) const {
  if(!mutation) {
    throw std::invalid_argument("HuxerUI Interaction: RevealOnTap requires a mutation");
  }
  if(!state_.has_value()) {
    throw std::logic_error("HuxerUI Interaction: RevealOnTapHandle is unbound");
  }
  state_->handle.RunFromCurrentInteraction(state_->transition, std::move(mutation));
}

RevealOnTapHandle UseRevealOnTap(RevealOnTapStyle style) {
  return RevealOnTapHandle(UseSceneTransition(), std::move(style.transition));
}

} // namespace huxerui::interaction
