#pragma once

#include <huxerui/animation.h>
#include <huxerui/event.h>
#include <huxerui/gesture.h>
#include <huxerui/interaction/pipe.h>
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
/// @throws std::invalid_argument If `dragging_opacity` is non-finite or outside [0, 1].
template <class T>
View Draggable(View content, T payload, DraggableStyle style = {},
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

/// Pipeable adaptor for Draggable<T>.
template <class T>
[[nodiscard]] ViewAdaptor Draggable(T payload, DraggableStyle style = {},
                                    std::function<View()> preview = {}, DragGesture gesture = {}) {
  return ViewAdaptor([payload = std::move(payload), style, preview = std::move(preview), gesture](View content) {
    return Draggable<T>(std::move(content), payload, style, preview, gesture);
  });
}

} // namespace huxerui::interaction
