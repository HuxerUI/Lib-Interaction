#pragma once

#include <huxerui/color.h>
#include <huxerui/geometry.h>
#include <huxerui/gesture.h>
#include <huxerui/modifier.h>
#include <huxerui/state.h>
#include <huxerui/view.h>

#include <cmath>
#include <functional>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {

/// Presentation configuration for DropZone.
struct DropZoneStyle {
  /// Border color while a compatible payload is hovering over the zone.
  Color highlight_color = Color::Rgb(31, 111, 235, 0.9F);
  /// Border width while highlighted, in logical pixels. Must be finite and non-negative.
  float border_width = 2.0F;
  bool operator==(const DropZoneStyle&) const = default;
};

/// Accepts an exact typed DragSource payload and paints a highlight border while it is over the zone.
///
/// The wrapper attaches DropTarget and observes Entered/Moved/Exited/Dropped without consuming pointer events. The
/// callback receives the borrowed payload and the node-local DropEvent; copy the payload if it must outlive the
/// callback. The zone imposes no intrinsic frame.
/// @param content Content that acts as the drop target.
/// @param on_drop Optional callback invoked once per accepted drop.
/// @param style Highlight color and border width.
/// @return A View carrying the DropTarget modifier and highlight presentation.
/// @throws std::invalid_argument If the border width is invalid.
template <class T>
View DropZone(View content, std::function<void(const T&, const DropEvent&)> on_drop = {}, DropZoneStyle style = {}) {
  if(!std::isfinite(style.border_width) || style.border_width < 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: DropZone border width must be finite and non-negative");
  }
  return Scope([content = std::move(content), on_drop = std::move(on_drop), style]() -> View {
    auto hovered = UseState(false);
    View zone = content;
    zone = std::move(zone)
               .With(DropTarget::template Accepts<T>())
               .template On<typename DropEvents<T>::Entered>([hovered](const T&, const DropEvent&) { hovered = true; })
               .template On<typename DropEvents<T>::Moved>([hovered](const T&, const DropEvent&) { hovered = true; })
               .template On<typename DropEvents<T>::Exited>([hovered](const T&, const DropEvent&) { hovered = false; })
               .template On<typename DropEvents<T>::Dropped>([hovered, on_drop](const T& payload, const DropEvent& event) {
                 hovered = false;
                 if(on_drop) on_drop(payload, event);
               })
               .With(Border{hovered.Get() ? style.highlight_color : Color::Transparent(),
                            hovered.Get() ? style.border_width : 0.0F});
    return zone;
  });
}

} // namespace huxerui::interaction
