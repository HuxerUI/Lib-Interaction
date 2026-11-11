#include <huxerui/interaction/pressdraghandle.h>

#include <huxerui/modifier.h>

#include <cmath>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {

View PressDragHandle(View content, PressDragHandleStyle style, PressDragHandleCallbacks callbacks) {
  if(!std::isfinite(style.press_duration.count()) || style.press_duration.count() <= 0.0) {
    throw std::invalid_argument("HuxerUI Interaction: PressDragHandle duration must be finite and positive");
  }
  View handle = std::move(content);
  handle = std::move(handle)
               .With(DragGesture{.axis = style.axis, .minimum_press_duration = style.press_duration})
               .On<DragEvents::Started>([callback = callbacks.on_started](const DragEvent& event) {
                 if(callback) callback(event);
               })
               .On<DragEvents::Changed>([callback = callbacks.on_changed](const DragEvent& event) {
                 if(callback) callback(event);
               })
               .On<DragEvents::Ended>([callback = callbacks.on_ended](const DragEvent& event) {
                 if(callback) callback(event);
               })
               .On<DragEvents::Canceled>([callback = callbacks.on_canceled](const DragEvent& event) {
                 if(callback) callback(event);
               });
  if(style.cursor != PointerCursorKind::Default) {
    handle = std::move(handle).With(PointerCursor{style.cursor});
  }
  return handle;
}

} // namespace huxerui::interaction
