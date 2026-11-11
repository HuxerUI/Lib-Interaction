#include <huxerui/interaction/secondaryclick.h>

#include <huxerui/modifier.h>

#include <utility>

namespace huxerui::interaction {

View SecondaryClickable(View content, std::function<void(Point)> on_secondary,
                        SecondaryClickableStyle style) {
  View clickable = std::move(content);
  clickable = std::move(clickable)
                  .On<ViewEvents::Pointer>([](const PointerEvent&) {})
                  .On<ViewEvents::ContextMenuRequested>(
                      [on_secondary = std::move(on_secondary)](Point position) {
                        if(on_secondary) on_secondary(position);
                      });
  if(style.cursor != PointerCursorKind::Default) {
    clickable = std::move(clickable).With(PointerCursor{style.cursor});
  }
  return clickable;
}

} // namespace huxerui::interaction
