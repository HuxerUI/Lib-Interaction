#pragma once

#include <huxerui/color.h>
#include <huxerui/geometry.h>
#include <huxerui/gesture.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/modifier.h>
#include <huxerui/state.h>
#include <huxerui/view.h>

#include <cmath>
#include <concepts>
#include <functional>
#include <stdexcept>
#include <type_traits>
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

/// Constrains payload predicates passed to predicate-filtered DropZone<T> overloads.
template <class T, class Predicate>
concept DropPayloadPredicate =
    std::same_as<T, std::remove_cvref_t<T>> && std::copy_constructible<Predicate> &&
    std::predicate<Predicate&, const T&> &&
    std::convertible_to<std::invoke_result_t<Predicate&, const T&>, bool> &&
    (!std::same_as<std::remove_cvref_t<Predicate>, View>) &&
    (!std::same_as<std::remove_cvref_t<Predicate>, DropZoneStyle>);

/// Headless predicate-filtered typed DropZone overload delegating hover presentation to `builder(bool hovered)`.
/// @throws std::invalid_argument If `builder` is empty.
template <class T, class Predicate>
  requires DropPayloadPredicate<T, Predicate>
View DropZone(std::function<View(bool hovered)> builder, Predicate accept,
              std::function<void(const T&, const DropEvent&)> on_drop = {}) {
  if(!builder) {
    throw std::invalid_argument("HuxerUI Interaction: DropZone requires a view builder");
  }
  return Scope([builder = std::move(builder), accept = std::move(accept), on_drop = std::move(on_drop)]() -> View {
    auto hovered = UseState(false);
    View zone = builder(hovered.Get());
    zone = std::move(zone)
               .With(DropTarget::template Accepts<T>(accept))
               .template On<typename DropEvents<T>::Entered>([hovered](const T&, const DropEvent&) { hovered = true; })
               .template On<typename DropEvents<T>::Moved>([hovered](const T&, const DropEvent&) { hovered = true; })
               .template On<typename DropEvents<T>::Exited>([hovered](const T&, const DropEvent&) { hovered = false; })
               .template On<typename DropEvents<T>::Dropped>(
                   [hovered, on_drop](const T& payload, const DropEvent& event) {
                     hovered = false;
                     if(on_drop) on_drop(payload, event);
                   });
    return zone;
  });
}

/// Headless typed DropZone overload delegating hover presentation to `builder(bool hovered)`.
/// @throws std::invalid_argument If `builder` is empty.
template <class T>
View DropZone(std::function<View(bool hovered)> builder,
              std::function<void(const T&, const DropEvent&)> on_drop = {}) {
  return DropZone<T>(std::move(builder), [](const T&) { return true; }, std::move(on_drop));
}

/// Accepts an exact typed DragSource payload matching `accept` and paints a highlight border while hovered.
/// @throws std::invalid_argument If `border_width` is non-finite or negative.
template <class T, class Predicate>
  requires DropPayloadPredicate<T, Predicate>
View DropZone(View content, Predicate accept, std::function<void(const T&, const DropEvent&)> on_drop = {},
              DropZoneStyle style = {}) {
  if(!std::isfinite(style.border_width) || style.border_width < 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: DropZone border width must be finite and non-negative");
  }
  return DropZone<T>(
      [content = std::move(content), style](bool hovered) -> View {
        View zone = content;
        return std::move(zone).With(Border{
            hovered ? style.highlight_color : Color::Transparent(),
            hovered ? style.border_width : 0.0F,
        });
      },
      std::move(accept), std::move(on_drop));
}

/// Accepts an exact typed DragSource payload and paints a highlight border while it is over the zone.
/// @throws std::invalid_argument If `border_width` is non-finite or negative.
template <class T>
View DropZone(View content, std::function<void(const T&, const DropEvent&)> on_drop = {}, DropZoneStyle style = {}) {
  return DropZone<T>(std::move(content), [](const T&) { return true; }, std::move(on_drop), std::move(style));
}

/// Pipeable adaptor for predicate-filtered DropZone<T>.
template <class T, class Predicate>
  requires DropPayloadPredicate<T, Predicate>
[[nodiscard]] ViewAdaptor DropZone(Predicate accept, std::function<void(const T&, const DropEvent&)> on_drop = {},
                                   DropZoneStyle style = {}) {
  return ViewAdaptor([accept = std::move(accept), on_drop = std::move(on_drop), style](View content) {
    return DropZone<T>(std::move(content), accept, on_drop, style);
  });
}

/// Pipeable adaptor for DropZone<T>.
template <class T>
[[nodiscard]] ViewAdaptor DropZone(std::function<void(const T&, const DropEvent&)> on_drop = {},
                                   DropZoneStyle style = {}) {
  return ViewAdaptor([on_drop = std::move(on_drop), style](View content) {
    return DropZone<T>(std::move(content), on_drop, style);
  });
}

} // namespace huxerui::interaction
