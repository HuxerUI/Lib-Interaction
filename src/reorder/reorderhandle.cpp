#include <huxerui/interaction/reorder/reorderhandle.h>

#include "../internal/reorder_core.h"
#include "../internal/validate.h"

#include <huxerui/environment.h>
#include <huxerui/modifier.h>

#include <utility>

namespace huxerui::interaction {

View ReorderHandle(View content, ReorderHandleStyle style) {
  internal::RequireOptionalPositiveDuration(
      style.minimum_press_duration, "HuxerUI Interaction: ReorderHandle press duration must be finite and positive");
  return Scope([content = std::move(content), style]() -> View {
    const auto& slot_env = UseEnvironment<internal::ReorderSlotEnvironment>();
    bool active_here = false;
    if(slot_env.context && slot_env.context->drag.IsValid()) {
      const internal::ReorderSession session = slot_env.context->drag.Get();
      active_here = session.active && session.from == slot_env.index;
    }
    View handle = content;
    if(slot_env.context) {
      handle = internal::BindReorderDrag(
          std::move(handle), slot_env.context, slot_env.index,
          DragGesture{
              .axis = std::nullopt,
              .minimum_distance = std::nullopt,
              .minimum_press_duration = style.minimum_press_duration,
          });
    }
    const PointerCursorKind effective_cursor = active_here ? PointerCursorKind::Grabbing : style.cursor;
    if(effective_cursor != PointerCursorKind::Default) {
      handle = std::move(handle).With(PointerCursor{effective_cursor});
    }
    return handle;
  });
}

} // namespace huxerui::interaction
