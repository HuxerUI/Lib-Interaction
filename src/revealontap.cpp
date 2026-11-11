#include <huxerui/interaction/revealontap.h>

#include <stdexcept>
#include <utility>

namespace huxerui::interaction {

RevealOnTapHandle::RevealOnTapHandle(SceneTransitionHandle handle, TransitionSpec transition)
    : handle_(std::move(handle)), transition_(std::move(transition)) {}

void RevealOnTapHandle::Run(std::function<void()> mutation) const {
  if(!mutation) {
    throw std::invalid_argument("HuxerUI Interaction: RevealOnTap requires a mutation");
  }
  handle_.RunFromCurrentInteraction(transition_, std::move(mutation));
}

RevealOnTapHandle UseRevealOnTap(RevealOnTapStyle style) {
  return RevealOnTapHandle(UseSceneTransition(), std::move(style.transition));
}

} // namespace huxerui::interaction
