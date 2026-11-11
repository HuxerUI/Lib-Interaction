#include <huxerui/interaction/hovercard.h>

#include <huxerui/layer.h>
#include <huxerui/state.h>
#include <huxerui/task.h>

#include <cmath>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const HoverCardStyle& style) {
  if(!std::isfinite(style.show_delay.count()) || style.show_delay.count() < 0.0) {
    throw std::invalid_argument("HuxerUI Interaction: HoverCard show delay must be finite and non-negative");
  }
  if(!std::isfinite(style.hide_delay.count()) || style.hide_delay.count() < 0.0) {
    throw std::invalid_argument("HuxerUI Interaction: HoverCard hide delay must be finite and non-negative");
  }
  if(style.long_press_duration.has_value()
     && (!std::isfinite(style.long_press_duration->count()) || style.long_press_duration->count() <= 0.0)) {
    throw std::invalid_argument("HuxerUI Interaction: HoverCard long press duration must be finite and positive");
  }
}

} // namespace

View HoverCard(View content, std::function<View(PopupContext)> card, HoverCardStyle style) {
  ValidateStyle(style);
  if(!card) {
    throw std::invalid_argument("HuxerUI Interaction: HoverCard requires a card factory");
  }
  return Scope([content = std::move(content), card = std::move(card), style]() -> View {
    PopupHandle popup = UsePopup();
    TaskScope tasks = UseTaskScope();
    auto layer = UseState<LayerId>(0);
    auto generation = UseState<std::uint64_t>(0);

    auto show = [popup, card, style, layer]() {
      if(layer.Get() != 0 && popup.Update(layer.Get(), card)) return;
      layer = popup.Show(card, style.popup_options);
    };
    auto hide = [popup, layer]() {
      if(layer.Get() == 0) return;
      popup.Dismiss(layer.Get());
      layer = 0;
    };

    View anchor = content;
    anchor = std::move(anchor).With(popup.Anchor());
    anchor = std::move(anchor)
                 .On<ViewEvents::Hover>([generation, tasks, style, show, hide](const HoverEvent& event) {
                   if(event.type == HoverEventType::Enter) {
                     const std::uint64_t token = generation.Get() + 1U;
                     generation = token;
                     tasks.Launch([generation, token, style, show]() -> Task<void> {
                       co_await Delay(style.show_delay);
                       if(generation.Get() != token) co_return;
                       show();
                     });
                   } else if(event.type == HoverEventType::Leave) {
                     const std::uint64_t token = generation.Get() + 1U;
                     generation = token;
                     tasks.Launch([generation, token, style, hide]() -> Task<void> {
                       co_await Delay(style.hide_delay);
                       if(generation.Get() != token) co_return;
                       hide();
                     });
                   }
                 });
    if(style.long_press) {
      anchor = std::move(anchor)
                   .With(LongPressGesture{.minimum_duration = style.long_press_duration})
                   .On<LongPressEvents::Started>([generation, show](const LongPressEvent&) {
                     generation = generation.Get() + 1U;
                     show();
                   })
                   .On<LongPressEvents::Ended>([generation, tasks, style, hide](const LongPressEvent&) {
                     const std::uint64_t token = generation.Get() + 1U;
                     generation = token;
                     tasks.Launch([generation, token, style, hide]() -> Task<void> {
                       co_await Delay(style.hide_delay);
                       if(generation.Get() != token) co_return;
                       hide();
                     });
                   })
                   .On<LongPressEvents::Canceled>([generation, hide](const LongPressEvent&) {
                     generation = generation.Get() + 1U;
                     hide();
                   });
    }
    return anchor;
  });
}

} // namespace huxerui::interaction
