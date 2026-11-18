#include <huxerui/interaction/press/surface.h>

#include <huxerui/modifier.h>
#include <huxerui/state.h>

#include <stdexcept>
#include <utility>

namespace huxerui::interaction {

View InteractiveSurface(std::function<View(const InteractionSnapshot&)> builder,
                        std::function<void()> on_click,
                        InteractiveSurfaceOptions options) {
  if(!builder) {
    throw std::invalid_argument("HuxerUI Interaction: InteractiveSurface requires a view builder");
  }
  return Scope([builder = std::move(builder), on_click = std::move(on_click), options]() -> View {
    auto hovered = UseState(false);
    auto pointer_pressed = UseState(false);
    auto keyboard_pressed = UseState(false);
    auto focused = UseState(false);
    const InteractionSnapshot snapshot{
        .hovered = hovered.Get(),
        .pressed = pointer_pressed.Get() || keyboard_pressed.Get(),
        .focused = focused.Get(),
    };

    View interactive = builder(snapshot);
    interactive = std::move(interactive)
                      .On<ViewEvents::Hover>([hovered](const HoverEvent& event) {
                        hovered = event.type != HoverEventType::Leave;
                      })
                      .On<ViewEvents::Pointer>([pointer_pressed](const PointerEvent& event) {
                        if(event.type == PointerEventType::Down && event.changed_button == PointerButton::Primary) {
                          pointer_pressed = true;
                        } else if(event.type == PointerEventType::Cancel
                                  || (event.type == PointerEventType::Up
                                      && event.changed_button == PointerButton::Primary)) {
                          pointer_pressed = false;
                        }
                      });
    if(on_click) {
      interactive = std::move(interactive).OnClick(on_click);
      if(options.role.has_value()) {
        interactive = std::move(interactive).With(Semantics{.role = *options.role});
      }
      interactive = std::move(interactive)
                        .On<ViewEvents::FocusChanged>([focused, keyboard_pressed](bool value) {
                          focused = value;
                          if(!value) keyboard_pressed = false;
                        });
      if(options.keyboard_focusable) {
        interactive = std::move(interactive)
                          .On<ViewEvents::KeyIntercept>([focused, keyboard_pressed](const KeyEvent& event) {
                            if(!focused.Get() || event.key != Key::Space || event.repeat) return false;
                            if(event.modifiers.shift || event.modifiers.control || event.modifiers.alt
                               || event.modifiers.meta) {
                              return false;
                            }
                            keyboard_pressed = event.type == KeyEventType::Down;
                            return false;
                          })
                          .With(Focusable{true});
      }
    }
    if(options.cursor != PointerCursorKind::Default) {
      interactive = std::move(interactive).With(PointerCursor{options.cursor});
    }
    return interactive;
  });
}

} // namespace huxerui::interaction
