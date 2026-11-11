#include <huxerui/interaction/scrollprogress.h>

#include <huxerui/modifier.h>
#include <huxerui/paint.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const ScrollProgressStyle& style) {
  if(!std::isfinite(style.thickness) || style.thickness <= 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: ScrollProgress thickness must be finite and positive");
  }
  if(!std::isfinite(style.inset) || style.inset < 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: ScrollProgress inset must be finite and non-negative");
  }
}

} // namespace

class ScrollProgress::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const ScrollProgress& value) { Update(node, value); }

  void Update(ViewNode&, const ScrollProgress& value) {
    controller_ = value.controller;
    style_ = value.style;
    InvalidatePaint(PaintInvalidation::Foreground);
  }

  void OnScrollActivity(ViewNode&, const ScrollActivity&) override {
    InvalidatePaint(PaintInvalidation::Foreground);
  }

  void PaintAboveContent(const ViewNode& node, PaintContext& context) const override {
    const ScrollMetrics metrics = controller_.Metrics();
    if(metrics.maximum_offset <= 0.0F || metrics.content_extent <= 0.0F || metrics.viewport_extent <= 0.0F) {
      return;
    }
    const Rect bounds = node.Bounds();
    if(bounds.IsEmpty()) return;
    const float track_length = (metrics.axis == Axis::Vertical ? bounds.height : bounds.width)
                               - style_.inset * 2.0F;
    if(track_length <= 0.0F) return;
    const float ratio = std::clamp(metrics.viewport_extent / metrics.content_extent, 0.0F, 1.0F);
    const float thumb_length = std::max(style_.thickness, track_length * ratio);
    const float progress = std::clamp(metrics.offset / metrics.maximum_offset, 0.0F, 1.0F);
    const float thumb_offset = progress * (track_length - thumb_length);
    const Color track_color = style_.track_color;
    if(metrics.axis == Axis::Vertical) {
      const float x = bounds.x + bounds.width - style_.inset - style_.thickness;
      context.DrawRect({x, bounds.y + style_.inset, style_.thickness, track_length}, track_color,
                       style_.corner_radii);
      context.DrawRect({x, bounds.y + style_.inset + thumb_offset, style_.thickness, thumb_length},
                       style_.fill_color, style_.corner_radii);
    } else {
      const float y = bounds.y + bounds.height - style_.inset - style_.thickness;
      context.DrawRect({bounds.x + style_.inset, y, track_length, style_.thickness}, track_color,
                       style_.corner_radii);
      context.DrawRect({bounds.x + style_.inset + thumb_offset, y, thumb_length, style_.thickness},
                       style_.fill_color, style_.corner_radii);
    }
  }

private:
  ScrollController controller_;
  ScrollProgressStyle style_;
};

const detail::ModifierDescriptor& ScrollProgress::Descriptor() {
  return detail::ModifierDescriptorFor<ScrollProgress, ScrollProgress::Extension>();
}

ScrollProgress::ScrollProgress(ScrollController controller, ScrollProgressStyle style)
    : controller(std::move(controller)), style(std::move(style)) {
  ValidateStyle(this->style);
}

} // namespace huxerui::interaction
