#include <huxerui/interaction/ripple.h>

#include <huxerui/modifier.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const RippleStyle& style) {
  if(!std::isfinite(style.duration) || style.duration <= 0.0) {
    throw std::invalid_argument("HuxerUI Interaction: Ripple duration must be finite and positive");
  }
  if(!std::isfinite(style.maximum_opacity) || style.maximum_opacity < 0.0F || style.maximum_opacity > 1.0F) {
    throw std::invalid_argument("HuxerUI Interaction: Ripple maximum opacity must be finite and in [0, 1]");
  }
}

float MaximumRadius(Point origin, const Rect& bounds) {
  const float horizontal = std::max(origin.x - bounds.x, bounds.x + bounds.width - origin.x);
  const float vertical = std::max(origin.y - bounds.y, bounds.y + bounds.height - origin.y);
  return std::sqrt(horizontal * horizontal + vertical * vertical);
}

} // namespace

/// Retained paint behavior for the Ripple modifier. The framework preserves this extension across recomposition
/// and calls Update() with the latest modifier value.
class Ripple::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const Ripple& value) { Update(node, value); }

  void Update(ViewNode&, const Ripple& value) {
    style_ = value.style;
    InvalidatePaint(PaintInvalidation::Foreground);
  }

  /// Lets the extension observe pointer sequences routed over the node without consuming them.
  [[nodiscard]] bool HitTest(ViewNode& node, Point position) const override {
    return node.Bounds().Contains(position);
  }

  PointerResult OnPointer(ViewNode&, const PointerEvent& event) override {
    if(event.type == PointerEventType::Down && event.changed_button == PointerButton::Primary) {
      origin_ = event.position;
      controller_.Set(0.0F);
      controller_.AnimateTo(1.0F, TweenSpec{.duration = style_.duration, .easing = Easing::EaseOut});
      InvalidatePaint(PaintInvalidation::Foreground);
      return PointerResult::Observe;
    }
    if(event.type == PointerEventType::Cancel && controller_.IsRunning()) {
      controller_.Set(1.0F);
      InvalidatePaint(PaintInvalidation::Foreground);
    }
    return PointerResult::Ignored;
  }

  FrameResult OnFrame(ViewNode&, const FrameInfo& frame) override {
    const MotionAdvanceResult result = controller_.Advance(frame);
    if(result.changed) InvalidatePaint(PaintInvalidation::Foreground);
    return {result.needs_frame, result.wake_after};
  }

  void PaintAboveContent(const ViewNode& node, PaintContext& context) const override {
    const float progress = controller_.Value();
    if(progress <= 0.0F || progress >= 1.0F) return;
    const Rect bounds = node.Bounds();
    if(bounds.IsEmpty()) return;
    Color color = style_.color;
    color.alpha *= style_.maximum_opacity * (1.0F - progress);
    if(color.alpha <= 0.0F) return;
    context.PushClip(bounds, node.ResolvedCornerRadii());
    context.DrawCircle(origin_, MaximumRadius(origin_, bounds) * progress, color);
    context.PopClip();
  }

private:
  RippleStyle style_;
  MotionController controller_{1.0F};
  Point origin_;
};

const detail::ModifierDescriptor& Ripple::Descriptor() {
  return detail::ModifierDescriptorFor<Ripple, Ripple::Extension>();
}

Ripple::Ripple(RippleStyle style) {
  ValidateStyle(style);
  this->style = std::move(style);
}

} // namespace huxerui::interaction
