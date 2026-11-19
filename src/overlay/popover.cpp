#include <huxerui/interaction/overlay/popover.h>

#include "../internal/validate.h"

#include <huxerui/lifecycle.h>
#include <huxerui/state.h>
#include <huxerui/task.h>

#include <stdexcept>
#include <utility>

namespace huxerui::interaction {

namespace detail {
struct PopoverControllerState {
  State<bool> open;
  std::function<void()> show_fn;
  std::function<void()> hide_fn;
};
} // namespace detail

namespace {

struct PopoverSession {
  bool alive = true;
  LayerId active_layer = 0;
};

void ValidateStyle(const PopoverStyle& style) {
  internal::RequireNonNegative(style.gap, "HuxerUI Interaction: Popover gap must be finite and non-negative");
  internal::RequireNonNegative(style.viewport_margin,
                               "HuxerUI Interaction: Popover viewport margin must be finite and non-negative");
  internal::RequireFinite(style.offset, "HuxerUI Interaction: Popover offset must be finite");
}

} // namespace

bool PopoverController::IsOpen() const {
  if(!state_ || !state_->open.IsValid()) return false;
  return state_->open.Get();
}

void PopoverController::Open() const {
  if(state_ && state_->show_fn) state_->show_fn();
}

void PopoverController::Close() const {
  if(state_ && state_->hide_fn) state_->hide_fn();
}

void PopoverController::Toggle() const {
  if(IsOpen()) {
    Close();
  } else {
    Open();
  }
}

PopoverController UsePopoverController() {
  auto open = UseState(false);
  auto holder = UseState(std::make_shared<detail::PopoverControllerState>());
  auto shared = holder.Get();
  shared->open = open;
  return PopoverController(std::move(shared));
}

View Popover(std::function<View(PopoverTriggerContext)> trigger, std::function<View(PopupContext)> content,
             PopoverStyle style, std::function<void(bool)> on_open_changed) {
  ValidateStyle(style);
  if(!trigger) {
    throw std::invalid_argument("HuxerUI Interaction: Popover requires a trigger builder");
  }
  if(!content) {
    throw std::invalid_argument("HuxerUI Interaction: Popover requires a content factory");
  }

  return Scope([trigger = std::move(trigger), content = std::move(content), style,
                on_open_changed = std::move(on_open_changed)]() -> View {
    PopupHandle popup = UsePopup();
    TaskScope tasks = UseTaskScope();
    auto session = UseState(std::make_shared<PopoverSession>()).Get();
    auto local_open = UseState(false);
    const State<bool> open_state =
        (style.controller.state_ && style.controller.state_->open.IsValid()) ? style.controller.state_->open
                                                                             : local_open;

    session->alive = true;
    Lifecycle([session, popup]() {
      session->alive = true;
      return [session, popup]() {
        session->alive = false;
        if(session->active_layer != 0) {
          const LayerId id = session->active_layer;
          session->active_layer = 0;
          popup.Dismiss(id);
        }
      };
    });

    auto notify_open = [open_state, on_open_changed](bool next) {
      if(!open_state.IsValid()) return;
      const bool prev = open_state.Get();
      if(prev != next) {
        open_state = next;
        if(on_open_changed) on_open_changed(next);
      }
    };

    auto build_popup = [content, session, tasks, notify_open](PopupContext ctx) -> View {
      return Scope([content, ctx, session, tasks, notify_open]() -> View {
        const LayerId popup_id = ctx.Id();
        Lifecycle([session, tasks, notify_open, popup_id]() {
          return [session, tasks, notify_open, popup_id]() {
            tasks.Post([session, notify_open, popup_id]() {
              if(!session->alive) return;
              if(session->active_layer == popup_id) {
                session->active_layer = 0;
                notify_open(false);
              }
            });
          };
        });
        return content(ctx);
      });
    };

    auto hide = [popup, session, notify_open]() {
      if(session->active_layer != 0) {
        const LayerId id = session->active_layer;
        session->active_layer = 0;
        popup.Dismiss(id);
      }
      notify_open(false);
    };

    auto show = [popup, build_popup, style, session, hide, notify_open]() {
      if(session->active_layer != 0 && popup.Update(session->active_layer, build_popup)) {
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
      const LayerId next_id = popup.Show(build_popup, std::move(options));
      session->active_layer = next_id;
      notify_open(next_id != 0);
    };

    auto toggle = [open_state, show, hide]() {
      if(open_state.IsValid() && open_state.Get()) {
        hide();
      } else {
        show();
      }
    };

    if(style.controller.state_) {
      style.controller.state_->show_fn = show;
      style.controller.state_->hide_fn = hide;
    }

    View anchor = trigger(PopoverTriggerContext{
        .open = open_state.Get(),
        .show = show,
        .hide = hide,
        .toggle = toggle,
    });
    return std::move(anchor).With(popup.Anchor());
  });
}

View Popover(View trigger, std::function<View(PopupContext)> content, PopoverStyle style,
             std::function<void(bool)> on_open_changed) {
  if(!content) {
    throw std::invalid_argument("HuxerUI Interaction: Popover requires a content factory");
  }
  return Popover(
      [trigger = std::move(trigger)](PopoverTriggerContext ctx) -> View {
        View view = trigger;
        return std::move(view).OnClick(std::move(ctx.toggle));
      },
      std::move(content), style, std::move(on_open_changed));
}

} // namespace huxerui::interaction
