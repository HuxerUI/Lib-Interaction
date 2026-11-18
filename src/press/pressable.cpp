#include <huxerui/interaction/press/pressable.h>
#include <huxerui/interaction/press/surface.h>

#include "../internal/validate.h"

#include <utility>

namespace huxerui::interaction::detail {
namespace {

void ValidateStyle(const PressableStyle& style) {
  internal::RequirePositive(style.hovered_scale,
                            "HuxerUI Interaction: Pressable scales must be finite and positive");
  internal::RequirePositive(style.pressed_scale,
                            "HuxerUI Interaction: Pressable scales must be finite and positive");
}

} // namespace

View PressableImpl(View content, std::function<void()> on_click, PressableStyle style) {
  ValidateStyle(style);
  return InteractiveSurface(
      [content = std::move(content), style](const InteractionSnapshot& state) -> View {
        const float target = state.pressed ? style.pressed_scale
                                           : (state.hovered ? style.hovered_scale : 1.0F);
        View scaled = content;
        return std::move(scaled).With(Scale(AnimateTo(target, style.animation), style.origin));
      },
      std::move(on_click),
      InteractiveSurfaceOptions{
          .cursor = style.cursor,
          .keyboard_focusable = style.keyboard_focusable,
      });
}

} // namespace huxerui::interaction::detail
