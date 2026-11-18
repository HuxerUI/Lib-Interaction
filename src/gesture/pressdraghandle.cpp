#include <huxerui/interaction/gesture/pressdraghandle.h>

#include "../internal/validate.h"

#include <huxerui/modifier.h>

#include <utility>

namespace huxerui::interaction {

View PressDragHandle(View content, PressDragHandleStyle style) {
  return PressDragHandle(std::move(content), PressDragHandleCallbacks{}, style);
}

View PressDragHandle(View content, PressDragHandleCallbacks callbacks, PressDragHandleStyle style) {
  internal::RequirePositiveDuration(
      style.press_duration, "HuxerUI Interaction: PressDragHandle duration must be finite and positive");
  View handle = std::move(content);
  handle = std::move(handle)
               .With(DragGesture{.axis = style.axis, .minimum_press_duration = style.press_duration})
               .On<DragEvents::Started>([callback = std::move(callbacks.on_started)](const DragEvent& event) {
                 if(callback) callback(event);
               })
               .On<DragEvents::Changed>([callback = std::move(callbacks.on_changed)](const DragEvent& event) {
                 if(callback) callback(event);
               })
               .On<DragEvents::Ended>([callback = std::move(callbacks.on_ended)](const DragEvent& event) {
                 if(callback) callback(event);
               })
               .On<DragEvents::Canceled>([callback = std::move(callbacks.on_canceled)](const DragEvent& event) {
                 if(callback) callback(event);
               });
  if(style.cursor != PointerCursorKind::Default) {
    handle = std::move(handle).With(PointerCursor{style.cursor});
  }
  return handle;
}

} // namespace huxerui::interaction
