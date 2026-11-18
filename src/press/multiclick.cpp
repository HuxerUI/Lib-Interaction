#include <huxerui/interaction/press/multiclick.h>

#include "../internal/validate.h"

#include <huxerui/modifier.h>
#include <huxerui/semantics.h>

#include <stdexcept>
#include <utility>

namespace huxerui::interaction::detail {
namespace {

void ValidateStyle(const MultiClickableStyle& style) {
  if(style.count < 2) {
    throw std::invalid_argument("HuxerUI Interaction: MultiClickable count must be at least two");
  }
  internal::RequireOptionalPositiveDuration(
      style.maximum_interval, "HuxerUI Interaction: MultiClickable interval must be finite and positive");
  internal::RequireOptionalPositive(
      style.maximum_movement, "HuxerUI Interaction: MultiClickable movement tolerance must be finite and positive");
}

} // namespace

View MultiClickableImpl(View content, std::function<void(const MultiTapEvent&)> on_recognized,
                        MultiClickableStyle style) {
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
  if(on_recognized && style.button_semantics) {
    clickable = std::move(clickable).With(Semantics{.role = SemanticRole::Button});
  }
  if(style.cursor != PointerCursorKind::Default) {
    clickable = std::move(clickable).With(PointerCursor{style.cursor});
  }
  return clickable;
}

View DoubleClickableImpl(View content, std::function<void(const MultiTapEvent&)> on_double_click,
                         DoubleClickableStyle style) {
  return MultiClickableImpl(
      std::move(content), std::move(on_double_click),
      MultiClickableStyle{
          .count = 2,
          .maximum_interval = style.maximum_interval,
          .maximum_movement = style.maximum_movement,
          .cursor = style.cursor,
          .button_semantics = style.button_semantics,
      });
}

} // namespace huxerui::interaction::detail
