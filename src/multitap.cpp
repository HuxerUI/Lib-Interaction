#include <huxerui/interaction/multitap.h>

#include <huxerui/modifier.h>
#include <huxerui/semantics.h>

#include <cmath>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const DoubleClickableStyle& style) {
  if(style.maximum_interval.has_value() && style.maximum_interval->count() <= 0.0) {
    throw std::invalid_argument("HuxerUI Interaction: DoubleClickable interval must be positive");
  }
  if(style.maximum_movement.has_value()
     && (!std::isfinite(*style.maximum_movement) || *style.maximum_movement <= 0.0F)) {
    throw std::invalid_argument("HuxerUI Interaction: DoubleClickable movement tolerance must be finite and positive");
  }
}

} // namespace

View DoubleClickable(View content, DoubleClickableStyle style,
                     std::function<void(const MultiTapEvent&)> on_double_click) {
  ValidateStyle(style);
  return Scope([content = std::move(content), style, on_double_click = std::move(on_double_click)]() -> View {
    View clickable = content;
    clickable = std::move(clickable)
                    .With(MultiTapGesture{
                        .count = 2,
                        .maximum_interval = style.maximum_interval,
                        .maximum_movement = style.maximum_movement,
                    })
                    .On<MultiTapEvents::Recognized>([on_double_click](const MultiTapEvent& event) {
                      if(on_double_click) on_double_click(event);
                    });
    if(on_double_click) {
      clickable = std::move(clickable).With(Semantics{.role = SemanticRole::Button});
    }
    if(style.cursor != PointerCursorKind::Default) {
      clickable = std::move(clickable).With(PointerCursor{style.cursor});
    }
    return clickable;
  });
}

} // namespace huxerui::interaction
