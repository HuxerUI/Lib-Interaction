#include <huxerui/interaction/ripple.h>

#include <huxerui/modifier.h>

#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace huxerui::interaction {
namespace {

// Bounds the per-frame cost of many simultaneous touches.
constexpr std::size_t kMaximumRipples = 4;

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
/// and calls Update() with the latest modifier value. Each active pointer owns one independent ripple.
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

  PointerResult OnPointer(ViewNode& node, const PointerEvent& event) override {
    if(event.type == PointerEventType::Down && event.changed_button == PointerButton::Primary) {
      if(ripples_.size() < kMaximumRipples) {
        ActiveRipple ripple;
        ripple.pointer_id = event.pointer_id;
        ripple.origin = event.position;
        if(style_.center_origin) {
          const Rect bounds = node.Bounds();
          ripple.origin = {bounds.x + bounds.width * 0.5F, bounds.y + bounds.height * 0.5F};
        }
        ripple.controller.AnimateTo(1.0F, TweenSpec{.duration = style_.duration, .easing = Easing::EaseOut});
        ripples_.push_back(std::move(ripple));
        InvalidatePaint(PaintInvalidation::Foreground);
      }
      return PointerResult::Observe;
    }
    if(event.type == PointerEventType::Cancel) {
      for(auto& ripple : ripples_) {
        if(ripple.pointer_id == event.pointer_id && ripple.controller.IsRunning()) {
          ripple.controller.Set(1.0F);
          InvalidatePaint(PaintInvalidation::Foreground);
        }
      }
    }
    return PointerResult::Ignored;
  }

  FrameResult OnFrame(ViewNode&, const FrameInfo& frame) override {
    bool changed = false;
    bool needs_frame = false;
    std::optional<double> wake_after;
    for(auto& ripple : ripples_) {
      const MotionAdvanceResult result = ripple.controller.Advance(frame);
      changed |= result.changed;
      needs_frame |= result.needs_frame;
      if(result.wake_after.has_value()) {
        wake_after = wake_after.has_value() ? std::min(*wake_after, *result.wake_after) : *result.wake_after;
      }
    }
    const auto settled = std::remove_if(ripples_.begin(), ripples_.end(),
                                        [](const ActiveRipple& ripple) {
                                          return !ripple.controller.IsRunning()
                                              && ripple.controller.Value() >= 1.0F;
                                        });
    if(settled != ripples_.end()) {
      ripples_.erase(settled, ripples_.end());
      changed = true;
    }
    if(changed) InvalidatePaint(PaintInvalidation::Foreground);
    return {needs_frame || !ripples_.empty(), wake_after};
  }

  void PaintAboveContent(const ViewNode& node, PaintContext& context) const override {
    if(ripples_.empty()) return;
    const Rect bounds = node.Bounds();
    if(bounds.IsEmpty()) return;
    for(const auto& ripple : ripples_) {
      const float progress = ripple.controller.Value();
      if(progress <= 0.0F || progress >= 1.0F) continue;
      Color color = style_.color;
      color.alpha *= style_.maximum_opacity * (1.0F - progress);
      if(color.alpha <= 0.0F) continue;
      context.PushClip(bounds, style_.corner_radii);
      context.DrawCircle(ripple.origin, MaximumRadius(ripple.origin, bounds) * progress, color);
      context.PopClip();
    }
  }

private:
  struct ActiveRipple {
    std::int64_t pointer_id = 0;
    Point origin;
    MotionController controller{0.0F};
  };

  RippleStyle style_;
  std::vector<ActiveRipple> ripples_;
};

const detail::ModifierDescriptor& Ripple::Descriptor() {
  return detail::ModifierDescriptorFor<Ripple, Ripple::Extension>();
}

Ripple::Ripple(RippleStyle style) {
  ValidateStyle(style);
  this->style = std::move(style);
}

} // namespace huxerui::interaction
