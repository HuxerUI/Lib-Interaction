#include <huxerui/interaction/overlay/hovercard.h>

#include "../internal/validate.h"

#include <huxerui/layer.h>
#include <huxerui/state.h>
#include <huxerui/task.h>

#include <cstdint>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {

namespace detail {
struct HoverCardControllerState {
  State<bool> open;
  std::function<void()> show_fn;
  std::function<void()> hide_fn;
};
} // namespace detail

namespace {

void ValidateStyle(const HoverCardStyle& style) {
  internal::RequireNonNegativeDuration(
      style.show_delay, "HuxerUI Interaction: HoverCard show delay must be finite and non-negative");
  internal::RequireNonNegativeDuration(
      style.hide_delay, "HuxerUI Interaction: HoverCard hide delay must be finite and non-negative");
  internal::RequireOptionalPositiveDuration(
      style.long_press_duration, "HuxerUI Interaction: HoverCard long press duration must be finite and positive");
  internal::RequireNonNegative(style.gap, "HuxerUI Interaction: HoverCard gap must be finite and non-negative");
  internal::RequireNonNegative(style.viewport_margin,
                               "HuxerUI Interaction: HoverCard viewport margin must be finite and non-negative");
  internal::RequireFinite(style.offset, "HuxerUI Interaction: HoverCard offset must be finite");
}

} // namespace

bool HoverCardController::IsOpen() const {
  if(!state_ || !state_->open.IsValid()) return false;
  return state_->open.Get();
}

void HoverCardController::Show() const {
  if(state_ && state_->show_fn) state_->show_fn();
}

void HoverCardController::Hide() const {
  if(state_ && state_->hide_fn) state_->hide_fn();
}

HoverCardController UseHoverCardController() {
  auto open = UseState(false);
  auto holder = UseState(std::make_shared<detail::HoverCardControllerState>());
  auto shared = holder.Get();
  shared->open = open;
  return HoverCardController(std::move(shared));
}

View HoverCard(View content, std::function<View(PopupContext)> card, HoverCardStyle style,
               std::function<void(bool)> on_open_changed) {
  ValidateStyle(style);
  if(!card) {
    throw std::invalid_argument("HuxerUI Interaction: HoverCard requires a card factory");
  }
  return Scope([content = std::move(content), card = std::move(card), style,
                on_open_changed = std::move(on_open_changed)]() -> View {
    PopupHandle popup = UsePopup();
    TaskScope tasks = UseTaskScope();
    auto layer = UseState<LayerId>(0);
    auto generation = UseState<std::uint64_t>(0);
    auto local_open = UseState(false);
    const State<bool> open_state =
        (style.controller.state_ && style.controller.state_->open.IsValid()) ? style.controller.state_->open
                                                                             : local_open;

    auto notify_open = [open_state, on_open_changed](bool next) {
      const bool prev = open_state.Get();
      if(prev != next) {
        open_state = next;
        if(on_open_changed) on_open_changed(next);
      }
    };

    auto hide = [popup, layer, generation, notify_open]() {
      generation = generation.Get() + 1U;
      if(layer.Get() != 0) {
        popup.Dismiss(layer.Get());
        layer = 0;
      }
      notify_open(false);
    };

    auto show = [popup, card, style, layer, generation, hide, notify_open]() {
      generation = generation.Get() + 1U;
      if(layer.Get() != 0 && popup.Update(layer.Get(), card)) {
        notify_open(true);
        return;
      }
      PopupOptions options{
          .placement = style.placement,
          .gap = style.gap,
          .viewport_margin = style.viewport_margin,
          .offset = style.offset,
          .dismiss_on_outside_press = style.dismiss_on_outside_press,
          .dismiss_on_cancel = style.dismiss_on_cancel,
          .trap_focus = style.trap_focus,
          .retain_anchor_focus = style.retain_anchor_focus,
          .on_dismiss_request = hide,
      };
      layer = popup.Show(card, std::move(options));
      notify_open(layer.Get() != 0);
    };

    if(style.controller.state_) {
      style.controller.state_->show_fn = show;
      style.controller.state_->hide_fn = hide;
    }

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
                   .With(LongPressGesture{
                       .minimum_duration = style.long_press_duration,
                       .maximum_movement = std::nullopt,
                   })
                   .On<LongPressEvents::Started>([show](const LongPressEvent&) { show(); })
                   .On<LongPressEvents::Ended>([generation, tasks, style, hide](const LongPressEvent&) {
                     const std::uint64_t token = generation.Get() + 1U;
                     generation = token;
                     tasks.Launch([generation, token, style, hide]() -> Task<void> {
                       co_await Delay(style.hide_delay);
                       if(generation.Get() != token) co_return;
                       hide();
                     });
                   })
                   .On<LongPressEvents::Canceled>([hide](const LongPressEvent&) { hide(); });
    }
    return anchor;
  });
}

} // namespace huxerui::interaction
