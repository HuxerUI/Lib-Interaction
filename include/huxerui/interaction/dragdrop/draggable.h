#pragma once

#include <huxerui/animation.h>
#include <huxerui/event.h>
#include <huxerui/gesture.h>
#include <huxerui/modifier.h>
#include <huxerui/state.h>
#include <huxerui/view.h>

#include <cmath>
#include <functional>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {

/// Presentation configuration for Draggable.
struct DraggableStyle {
  /// Opacity of the source while its DragSource owns the drag. Must be finite and in [0, 1].
  float dragging_opacity = 0.55F;
  /// Motion used when the source enters or leaves the dragging state.
  AnimationSpec animation = TweenSpec{.duration = 0.12, .easing = Easing::EaseOut};
  /// Cursor shown over the source. Default uses the framework's grab cursor.
  PointerCursorKind cursor = PointerCursorKind::Grab;
  bool operator==(const DraggableStyle&) const = default;
};

/// Makes content a typed in-process drag source.
///
/// The payload type is captured by DragSource and must match the DropTarget's exact payload type. A non-empty preview
/// factory supplies the drag preview; an empty factory lets the framework use its default source preview. The source
/// fades to dragging_opacity while its DragSource owns the drag. DropTarget handles routing; this wrapper only adds
/// presentation.
/// @param content Content that starts the drag.
/// @param payload Value copied into the typed drag session.
/// @param style Fade amount, motion, and cursor.
/// @param preview Optional drag-preview builder.
/// @param gesture Optional drag recognition override. The default DragGesture is used when omitted.
/// @return A View carrying the DragSource modifier and source presentation.
/// @throws std::invalid_argument If the opacity is outside [0, 1].
template <class T> View Draggable(View content, T payload, DraggableStyle style = {},
                                  std::function<View()> preview = {}, DragGesture gesture = {}) {
  if(!std::isfinite(style.dragging_opacity) || style.dragging_opacity < 0.0F || style.dragging_opacity > 1.0F) {
    throw std::invalid_argument("HuxerUI Interaction: Draggable opacity must be finite and in [0, 1]");
  }
  return Scope([content = std::move(content), payload = std::move(payload), style,
                preview = std::move(preview), gesture]() -> View {
    auto dragging = UseState(false);
    View source = content;
    if(preview) {
      source = std::move(source).With(DragSource(std::move(payload), preview, gesture));
    } else {
      source = std::move(source).With(DragSource(std::move(payload), gesture));
    }
    source = std::move(source)
                 .With(Opacity(AnimateTo(dragging.Get() ? style.dragging_opacity : 1.0F, style.animation)))
                 .template On<DragSourceEvents::Started>([dragging](const DragEvent&) { dragging = true; })
                 .template On<DragSourceEvents::Ended>([dragging](const DragDropResult&) { dragging = false; })
                 .template On<DragSourceEvents::Canceled>([dragging](const DragEvent&) { dragging = false; });
    if(style.cursor != PointerCursorKind::Default) {
      source = std::move(source).With(PointerCursor{style.cursor});
    }
    return source;
  });
}

} // namespace huxerui::interaction
