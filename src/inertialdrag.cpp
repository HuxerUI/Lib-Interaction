#include <huxerui/interaction/inertialdrag.h>

#include <huxerui/state.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const InertialDragStyle& style) {
  if(!std::isfinite(style.projection_time) || style.projection_time < 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: InertialDrag projection time must be finite and non-negative");
  }
  if(!std::isfinite(style.maximum_projection) || style.maximum_projection < 0.0F) {
    throw std::invalid_argument(
        "HuxerUI Interaction: InertialDrag maximum projection must be finite and non-negative");
  }
}

float ClampProjection(float value, float limit) { return std::clamp(value, -limit, limit); }

} // namespace

View InertialDrag(View content, InertialDragStyle style) {
  ValidateStyle(style);
  return Scope([content = std::move(content), style]() -> View {
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
        .On<DragEvents::Ended>([dragging, offset, style](const DragEvent& event) {
          if(!dragging.Get()) return;
          Point projected{event.translation.x + event.velocity.x * style.projection_time,
                          event.translation.y + event.velocity.y * style.projection_time};
          projected.x = ClampProjection(projected.x, style.maximum_projection);
          projected.y = ClampProjection(projected.y, style.maximum_projection);
          offset = projected;
          dragging = false;
        })
        .On<DragEvents::Canceled>([dragging, offset](const DragEvent&) {
          dragging = false;
          offset = Point{};
        });
  });
}

} // namespace huxerui::interaction
