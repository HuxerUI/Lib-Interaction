#include <huxerui/interaction/pressable.h>

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
    auto pressed = UseState(false);
    const float target = pressed ? style.pressed_scale : (hovered ? style.hovered_scale : 1.0F);
    View interactive = content;
    interactive = std::move(interactive)
                      .With(Scale(AnimateTo(target, style.animation), style.origin))
                      .On<ViewEvents::Hover>([hovered](const HoverEvent& event) {
                        hovered = event.type != HoverEventType::Leave;
                      })
                      .On<ViewEvents::Pointer>([pressed](const PointerEvent& event) {
                        if(event.type == PointerEventType::Down) {
                          pressed = true;
                        } else if(event.type == PointerEventType::Up || event.type == PointerEventType::Cancel) {
                          pressed = false;
                        }
                      });
    if(on_click) interactive = std::move(interactive).OnClick(on_click);
    return interactive;
  });
}

} // namespace huxerui::interaction
