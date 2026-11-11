#include <huxerui/interaction/resizehandle.h>

#include <huxerui/event.h>
#include <huxerui/gesture.h>
#include <huxerui/modifier.h>
#include <huxerui/state.h>

#include <cmath>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {

View ResizeHandle(Axis axis, std::function<void(float delta)> on_resize, ResizeHandleStyle style) {
  if(!std::isfinite(style.thickness) || style.thickness <= 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: ResizeHandle thickness must be finite and positive");
  }
  return Scope([axis, on_resize = std::move(on_resize), style]() -> View {
    auto pressed = UseState(false);
    View handle = Spacer();
    const Frame frame = axis == Axis::Horizontal ? Frame{.width = style.thickness}
                                                 : Frame{.height = style.thickness};
    handle = std::move(handle)
                 .With(frame, Background{pressed.Get() ? style.pressed_color : style.color},
                       DragGesture{.axis = axis},
                       PointerCursor{axis == Axis::Horizontal ? PointerCursorKind::ResizeHorizontal
                                                               : PointerCursorKind::ResizeVertical})
                 .On<DragEvents::Started>([pressed](const DragEvent&) { pressed = true; })
                 .On<DragEvents::Changed>([on_resize, axis](const DragEvent& event) {
                   if(on_resize) on_resize(axis == Axis::Horizontal ? event.delta.x : event.delta.y);
                 })
                 .On<DragEvents::Ended>([pressed](const DragEvent&) { pressed = false; })
                 .On<DragEvents::Canceled>([pressed](const DragEvent&) { pressed = false; });
    return handle;
  });
}

} // namespace huxerui::interaction
