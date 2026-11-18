#include <huxerui/interaction/gesture/inertialdrag.h>

#include "../internal/validate.h"

#include <huxerui/state.h>

#include <algorithm>
#include <utility>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const InertialDragStyle& style) {
  internal::RequireNonNegative(
      style.projection_time, "HuxerUI Interaction: InertialDrag projection time must be finite and non-negative");
  internal::RequireNonNegative(
      style.maximum_projection, "HuxerUI Interaction: InertialDrag maximum projection must be finite and non-negative");
}

float ClampProjection(float value, float limit) { return std::clamp(value, -limit, limit); }

} // namespace

View InertialDrag(View content, InertialDragStyle style, std::function<void(Point)> on_settled) {
  ValidateStyle(style);
  return Scope([content = std::move(content), style, on_settled = std::move(on_settled)]() -> View {
    auto offset = UseState(Point{});
    auto dragging = UseState(false);
    const AnimationSpec motion = dragging.Get() ? style.follow_motion : style.settle_motion;
    View draggable = content;
    return std::move(draggable)
        .With(DragGesture{.axis = style.axis})
        .With(Offset(AnimateTo(offset.Get(), motion)))
        .On<DragEvents::Started>([dragging](const DragEvent&) { dragging = true; })
        .On<DragEvents::Changed>([dragging, offset](const DragEvent& event) {
          if(!dragging.Get()) return;
          offset = event.translation;
        })
        .On<DragEvents::Ended>([dragging, offset, style, on_settled](const DragEvent& event) {
          if(!dragging.Get()) return;
          Point projected{event.translation.x + event.velocity.x * style.projection_time,
                          event.translation.y + event.velocity.y * style.projection_time};
          projected.x = ClampProjection(projected.x, style.maximum_projection);
          projected.y = ClampProjection(projected.y, style.maximum_projection);
          offset = projected;
          dragging = false;
          if(on_settled) on_settled(projected);
        })
        .On<DragEvents::Canceled>([dragging, offset](const DragEvent&) {
          dragging = false;
          offset = Point{};
        });
  });
}

} // namespace huxerui::interaction
