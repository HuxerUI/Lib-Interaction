#include <huxerui/interaction/hover/spotlight.h>

#include "../internal/validate.h"

#include <huxerui/modifier.h>
#include <huxerui/paint.h>

#include <utility>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const SpotlightStyle& style) {
  internal::RequirePositive(style.diameter, "HuxerUI Interaction: Spotlight diameter must be finite and positive");
  internal::RequireNonNegative(style.fade, "HuxerUI Interaction: Spotlight fade must be finite and non-negative");
}

} // namespace

class Spotlight::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const Spotlight& value) { Update(node, value); }

  void Update(ViewNode&, const Spotlight& value) {
    style_ = value.style;
    InvalidatePaint(PaintInvalidation::Content);
  }

  [[nodiscard]] bool HoverHitTest(ViewNode& node, Point position) const override {
    return node.Bounds().Contains(position);
  }

  void OnHover(ViewNode&, const HoverEvent& event) override {
    if(event.type == HoverEventType::Leave) {
      if(presence_.Target() != 0.0F) {
        presence_.AnimateTo(0.0F, TweenSpec{.duration = style_.fade, .easing = Easing::EaseOut});
        InvalidatePaint(PaintInvalidation::Content);
      }
      return;
    }
    position_ = event.position;
    if(presence_.Target() != 1.0F) {
      presence_.AnimateTo(1.0F, TweenSpec{.duration = style_.fade, .easing = Easing::EaseOut});
    }
    InvalidatePaint(PaintInvalidation::Content);
  }

  FrameResult OnFrame(ViewNode&, const FrameInfo& frame) override {
    const MotionAdvanceResult result = presence_.Advance(frame);
    if(result.changed) InvalidatePaint(PaintInvalidation::Content);
    return {result.needs_frame, result.wake_after};
  }

  void PaintBehindContent(const ViewNode& node, PaintContext& context) const override {
    const float presence = presence_.Value();
    if(presence <= 0.0F) return;
    const Rect bounds = node.Bounds();
    if(bounds.IsEmpty() || bounds.width <= 0.0F || bounds.height <= 0.0F) return;
    Color center = style_.color;
    center.alpha *= presence;
    if(center.alpha <= 0.0F) return;
    const RadialGradient glow{
        .center = {(position_.x - bounds.x) / bounds.width, (position_.y - bounds.y) / bounds.height},
        .radius = {style_.diameter / bounds.width, style_.diameter / bounds.height},
        .stops = {{.offset = 0.0F, .color = center}, {.offset = 1.0F, .color = Color::Transparent()}},
    };
    context.PushClip(bounds, style_.corner_radii);
    context.DrawRect(bounds, Brush(glow), style_.corner_radii);
    context.PopClip();
  }

private:
  SpotlightStyle style_;
  MotionController presence_{0.0F};
  Point position_;
};

const huxerui::detail::ModifierDescriptor& Spotlight::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<Spotlight, Spotlight::Extension>();
}

Spotlight::Spotlight(SpotlightStyle style) {
  ValidateStyle(style);
  this->style = std::move(style);
}

} // namespace huxerui::interaction
