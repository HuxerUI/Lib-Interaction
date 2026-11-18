#include <huxerui/interaction/press/contextmenu.h>

#include "../internal/validate.h"

#include <huxerui/modifier.h>

#include <utility>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const ContextMenuAreaStyle& style) {
  internal::RequireOptionalPositiveDuration(
      style.long_press_duration, "HuxerUI Interaction: ContextMenuArea duration must be finite and positive");
}

/// Participation-only modifier so inert views receive secondary-click routing without overwriting ViewEvents::Pointer.
struct ContextHitTarget {
  class Extension;
  static const huxerui::detail::ModifierDescriptor& Descriptor();
  bool operator==(const ContextHitTarget&) const = default;
};

class ContextHitTarget::Extension final : public NodeExtension {
public:
  Extension(ViewNode&, const ContextHitTarget&) {}
  void Update(ViewNode&, const ContextHitTarget&) {}

  [[nodiscard]] bool HitTest(ViewNode& node, Point position) const override {
    return node.Bounds().Contains(position);
  }
};

const huxerui::detail::ModifierDescriptor& ContextHitTarget::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<ContextHitTarget, ContextHitTarget::Extension>();
}

} // namespace

View ContextMenuArea(View content, std::function<void(Point)> on_request, ContextMenuAreaStyle style) {
  ValidateStyle(style);
  View area = std::move(content);
  if(style.hit_test_inert) {
    area = std::move(area).With(ContextHitTarget{});
  }
  area = std::move(area).On<ViewEvents::ContextMenuRequested>([on_request](Point position) {
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
  if(style.cursor != PointerCursorKind::Default) {
    area = std::move(area).With(PointerCursor{style.cursor});
  }
  return area;
}

} // namespace huxerui::interaction
