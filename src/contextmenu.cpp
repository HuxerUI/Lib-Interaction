#include <huxerui/interaction/contextmenu.h>

#include <stdexcept>
#include <utility>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const ContextMenuAreaStyle& style) {
  if(style.long_press_duration.has_value() && style.long_press_duration->count() <= 0.0) {
    throw std::invalid_argument("HuxerUI Interaction: ContextMenuArea duration must be positive");
  }
}

} // namespace

View ContextMenuArea(View content, std::function<void(Point)> on_request, ContextMenuAreaStyle style) {
  ValidateStyle(style);
  View area = std::move(content);
  area = std::move(area).On<ViewEvents::ContextMenuRequested>(
      [on_request](Point position) {
        if(on_request) on_request(position);
      });
  if(style.long_press) {
    area = std::move(area)
               .With(LongPressGesture{
                   .minimum_duration = style.long_press_duration,
                   .maximum_movement = std::nullopt,
               })
               .On<LongPressEvents::Started>([on_request](const LongPressEvent& event) {
                 if(on_request && event.device_kind == PointerDeviceKind::Touch) {
                   on_request(event.window_position);
                 }
               });
  }
  return area;
}

} // namespace huxerui::interaction
