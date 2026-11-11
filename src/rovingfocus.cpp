#include <huxerui/interaction/rovingfocus.h>

#include <huxerui/event.h>
#include <huxerui/state.h>

#include <cmath>
#include <optional>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {

View RovingFocus(std::vector<View> items, RovingFocusStyle style) {
  if(!std::isfinite(style.spacing) || style.spacing < 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: RovingFocus spacing must be finite and non-negative");
  }
  return Scope([items = std::move(items), style]() -> View {
    const std::size_t count = items.size();
    std::vector<FocusRequester> requesters;
    requesters.reserve(count);
    for(std::size_t index = 0; index < count; ++index) {
      requesters.push_back(UseFocusRequester());
    }
    auto focused_index = UseState<std::size_t>(0);
    std::vector<View> focusable;
    focusable.reserve(count);
    for(std::size_t index = 0; index < count; ++index) {
      View item = std::move(items[index]);
      item = std::move(item)
                 .With(Focusable{true}, FocusTarget{requesters[index]})
                 .On<ViewEvents::FocusChanged>([focused_index, index](bool focused) {
                   if(focused) focused_index = index;
                 });
      focusable.push_back(std::move(item));
    }
    View list = style.axis == Axis::Vertical ? View(Column(std::move(focusable))) : View(Row(std::move(focusable)));
    return std::move(list)
        .With(Spacing(style.spacing))
        .On<ViewEvents::KeyIntercept>([focused_index, requesters, count, style](const KeyEvent& event) {
          if(event.type != KeyEventType::Down || event.repeat || count == 0) return false;
          std::optional<int> delta;
          if(style.axis == Axis::Vertical) {
            if(event.key == Key::ArrowUp) delta = -1;
            if(event.key == Key::ArrowDown) delta = 1;
          } else {
            if(event.key == Key::ArrowLeft) delta = -1;
            if(event.key == Key::ArrowRight) delta = 1;
          }
          if(!delta.has_value()) return false;
          long target = static_cast<long>(focused_index.Get()) + *delta;
          if(target < 0 || target >= static_cast<long>(count)) {
            if(!style.wrap) return true;
            target = (target + static_cast<long>(count)) % static_cast<long>(count);
          }
          return requesters[static_cast<std::size_t>(target)].RequestFocus();
        });
  });
}

} // namespace huxerui::interaction
