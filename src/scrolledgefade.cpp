#include <huxerui/interaction/scrolledgefade.h>

#include <huxerui/paint.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const ScrollEdgeFadeStyle& style) {
  if(!std::isfinite(style.extent) || style.extent <= 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: ScrollEdgeFade extent must be finite and positive");
  }
}

Color Transparent() { return Color::Transparent(); }

} // namespace

class ScrollEdgeFade::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const ScrollEdgeFade& value) { Update(node, value); }

  void Update(ViewNode&, const ScrollEdgeFade& value) {
    controller_ = value.controller;
    style_ = value.style;
    InvalidatePaint(PaintInvalidation::Foreground);
  }

  void OnScrollActivity(ViewNode&, const ScrollActivity&) override {
    InvalidatePaint(PaintInvalidation::Foreground);
  }

  void PaintAboveContent(const ViewNode& node, PaintContext& context) const override {
    const ScrollMetrics metrics = controller_.Metrics();
    if(metrics.maximum_offset <= 0.0F) return;
    const Rect bounds = node.Bounds();
    if(bounds.IsEmpty()) return;
    const bool vertical = metrics.axis == Axis::Vertical;
    const float length = vertical ? bounds.height : bounds.width;
    const float extent = std::min(style_.extent, length * 0.5F);
    if(extent <= 0.0F) return;

    if(style_.fade_start && metrics.offset > 0.0F) {
      const LinearGradient gradient{
          .start = {0.0F, 0.0F},
          .end = {vertical ? 0.0F : 1.0F, vertical ? 1.0F : 0.0F},
          .stops = {{0.0F, style_.color}, {1.0F, Transparent()}},
      };
      const Rect rect = vertical ? Rect{bounds.x, bounds.y, bounds.width, extent}
                                 : Rect{bounds.x, bounds.y, extent, bounds.height};
      context.DrawRect(rect, Brush(gradient));
    }
    if(style_.fade_end && metrics.offset < metrics.maximum_offset) {
      const LinearGradient gradient{
          .start = {0.0F, 0.0F},
          .end = {vertical ? 0.0F : 1.0F, vertical ? 1.0F : 0.0F},
          .stops = {{0.0F, Transparent()}, {1.0F, style_.color}},
      };
      const Rect rect = vertical
                            ? Rect{bounds.x, bounds.y + bounds.height - extent, bounds.width, extent}
                            : Rect{bounds.x + bounds.width - extent, bounds.y, extent, bounds.height};
      context.DrawRect(rect, Brush(gradient));
    }
  }

private:
  ScrollController controller_;
  ScrollEdgeFadeStyle style_;
};

const detail::ModifierDescriptor& ScrollEdgeFade::Descriptor() {
  return detail::ModifierDescriptorFor<ScrollEdgeFade, ScrollEdgeFade::Extension>();
}

ScrollEdgeFade::ScrollEdgeFade(ScrollController controller, ScrollEdgeFadeStyle style)
    : controller(std::move(controller)), style(std::move(style)) {
  ValidateStyle(this->style);
}

} // namespace huxerui::interaction
