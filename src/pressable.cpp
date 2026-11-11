#include <huxerui/interaction/pressable.h>

#include <huxerui/modifier.h>
#include <huxerui/semantics.h>
#include <huxerui/state.h>

#include <cmath>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const PressableStyle& style) {
  for(float scale : {style.hovered_scale, style.pressed_scale}) {
    if(!std::isfinite(scale) || scale <= 0.0F) {
      throw std::invalid_argument("HuxerUI Interaction: Pressable scales must be finite and positive");
    }
  }
}

} // namespace

View Pressable(View content, PressableStyle style, std::function<void()> on_click) {
  ValidateStyle(style);
  return Scope([content = std::move(content), style, on_click = std::move(on_click)]() -> View {
    auto hovered = UseState(false);
    auto pointer_pressed = UseState(false);
    auto keyboard_pressed = UseState(false);
    auto focused = UseState(false);
    const bool pressed = pointer_pressed.Get() || keyboard_pressed.Get();
    const float target = pressed ? style.pressed_scale : (hovered ? style.hovered_scale : 1.0F);
    View interactive = content;
    interactive = std::move(interactive)
                      .With(Scale(AnimateTo(target, style.animation), style.origin))
                      .On<ViewEvents::Hover>([hovered](const HoverEvent& event) {
                        hovered = event.type != HoverEventType::Leave;
                      })
                      .On<ViewEvents::Pointer>([pointer_pressed](const PointerEvent& event) {
                        if(event.type == PointerEventType::Down) {
                          pointer_pressed = true;
                        } else if(event.type == PointerEventType::Up || event.type == PointerEventType::Cancel) {
                          pointer_pressed = false;
                        }
                      });
    if(on_click) {
      interactive = std::move(interactive).OnClick(on_click);
      interactive = std::move(interactive).With(Semantics{.role = SemanticRole::Button});
      interactive = std::move(interactive)
                        .On<ViewEvents::FocusChanged>([focused, keyboard_pressed](bool value) {
                          focused = value;
                          if(!value) keyboard_pressed = false;
                        });
      if(style.keyboard_focusable) {
        interactive = std::move(interactive)
                          .On<ViewEvents::KeyIntercept>([focused, keyboard_pressed](const KeyEvent& event) {
                            if(!focused.Get() || event.key != Key::Space || event.repeat) return false;
                            if(event.modifiers.shift || event.modifiers.control || event.modifiers.alt
                               || event.modifiers.meta) {
                              return false;
                            }
                            keyboard_pressed = event.type == KeyEventType::Down;
                            return false;
                          });
      }
    }
    if(on_click && style.keyboard_focusable) {
      interactive = std::move(interactive).With(Focusable{true});
    }
    if(style.cursor != PointerCursorKind::Default) {
      interactive = std::move(interactive).With(PointerCursor{style.cursor});
    }
    return interactive;
  });
}

} // namespace huxerui::interaction
