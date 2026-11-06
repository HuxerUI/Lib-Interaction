#include <huxerui/interaction/dragspring.h>

#include <huxerui/state.h>

#include <utility>

namespace huxerui::interaction {

View DragSpring(View content, DragSpringStyle style) {
  return Scope([content = std::move(content), style]() -> View {
    auto dragging = UseState(false);
    auto offset = UseState(Point{});
    const Point target = dragging ? offset.Get() : Point{};
    const AnimationSpec motion = dragging
        ? AnimationSpec(TweenSpec{.duration = 0.04, .easing = Easing::Linear})
        : style.return_motion;
    View draggable = content;
    return std::move(draggable)
        .With(DragGesture{.axis = style.axis})
        .With(Offset(AnimateTo(target, motion)))
        .On<DragEvents::Started>([dragging](const DragEvent&) { dragging = true; })
        .On<DragEvents::Changed>([offset](const DragEvent& event) { offset = event.translation; })
        .On<DragEvents::Ended>([dragging](const DragEvent&) { dragging = false; })
        .On<DragEvents::Canceled>([dragging](const DragEvent&) { dragging = false; });
  });
}

} // namespace huxerui::interaction
