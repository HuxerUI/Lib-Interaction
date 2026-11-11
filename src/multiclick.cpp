#include <huxerui/interaction/multiclick.h>

#include <huxerui/modifier.h>

#include <cmath>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const MultiClickableStyle& style) {
  if(style.count < 2) {
    throw std::invalid_argument("HuxerUI Interaction: MultiClickable count must be at least two");
  }
  if(style.maximum_interval.has_value() && style.maximum_interval->count() <= 0.0) {
    throw std::invalid_argument("HuxerUI Interaction: MultiClickable interval must be positive");
  }
  if(style.maximum_movement.has_value()
     && (!std::isfinite(*style.maximum_movement) || *style.maximum_movement <= 0.0F)) {
    throw std::invalid_argument(
        "HuxerUI Interaction: MultiClickable movement tolerance must be finite and positive");
  }
}

} // namespace

View MultiClickable(View content, MultiClickableStyle style,
                    std::function<void(const MultiTapEvent&)> on_recognized) {
  ValidateStyle(style);
  View clickable = std::move(content);
  clickable = std::move(clickable)
                  .With(MultiTapGesture{
                      .count = style.count,
                      .maximum_interval = style.maximum_interval,
                      .maximum_movement = style.maximum_movement,
                  })
                  .On<MultiTapEvents::Recognized>([on_recognized](const MultiTapEvent& event) {
                    if(on_recognized) on_recognized(event);
                  });
  if(style.cursor != PointerCursorKind::Default) {
    clickable = std::move(clickable).With(PointerCursor{style.cursor});
  }
  return clickable;
}

} // namespace huxerui::interaction
