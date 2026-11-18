#include <huxerui/interaction/press/ripple.h>

#include "../internal/motion_group.h"
#include "../internal/validate.h"

#include <huxerui/event.h>
#include <huxerui/modifier.h>

#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>
#include <vector>

namespace huxerui::interaction {
namespace {

constexpr std::size_t kMaximumRipples = 4;

void ValidateStyle(const RippleStyle& style) {
  internal::RequirePositive(style.duration, "HuxerUI Interaction: Ripple duration must be finite and positive");
  internal::RequireUnitInterval(style.maximum_opacity,
                                "HuxerUI Interaction: Ripple maximum opacity must be finite and in [0, 1]");
}

float MaximumRadius(Point origin, const Rect& bounds) {
  const float horizontal = std::max(origin.x - bounds.x, bounds.x + bounds.width - origin.x);
  const float vertical = std::max(origin.y - bounds.y, bounds.y + bounds.height - origin.y);
  return std::sqrt(horizontal * horizontal + vertical * vertical);
}

} // namespace

class Ripple::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const Ripple& value) { Update(node, value); }

  void Update(ViewNode&, const Ripple& value) {
    style_ = value.style;
    InvalidatePaint(PaintInvalidation::Foreground);
  }

  [[nodiscard]] bool HitTest(ViewNode& node, Point position) const override {
    return node.Bounds().Contains(position);
  }

  PointerResult OnPointer(ViewNode&, const PointerEvent& event) override {
    if(event.type == PointerEventType::Down && event.changed_button == PointerButton::Primary) {
      return PointerResult::Observe;
    }
    return PointerResult::Ignored;
  }

  void OnInteraction(ViewNode& node, const InteractionState& state,
                     const std::optional<InteractionEvent>& event) override {
    if(!state.enabled) {
      CancelAll();
      return;
    }
    if(!event.has_value()) return;
    if(event->type == InteractionEvent::Type::Press) {
      Start(node, *event);
    } else if(event->type == InteractionEvent::Type::Cancel) {
      Finish(event->press_id);
    }
  }

  FrameResult OnFrame(ViewNode&, const FrameInfo& frame) override {
    internal::FrameAccumulator acc;
    for(auto& ripple : ripples_) {
      acc.Add(ripple.controller.Advance(frame));
    }
    const auto settled = std::remove_if(ripples_.begin(), ripples_.end(), [](const ActiveRipple& ripple) {
      return !ripple.controller.IsRunning() && ripple.controller.Value() >= 1.0F;
    });
    if(settled != ripples_.end()) {
      ripples_.erase(settled, ripples_.end());
      acc.changed = true;
    }
    if(acc.changed) InvalidatePaint(PaintInvalidation::Foreground);
    return acc.Result(!ripples_.empty());
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
    std::uint64_t press_id = 0;
    Point origin;
    MotionController controller{0.0F};
  };

  Point OriginFor(ViewNode& node, const InteractionEvent& event) const {
    const Rect bounds = node.Bounds();
    if(style_.center_origin || !event.position.has_value()) {
      return {bounds.x + bounds.width * 0.5F, bounds.y + bounds.height * 0.5F};
    }
    return *event.position;
  }

  void Start(ViewNode& node, const InteractionEvent& event) {
    if(ripples_.size() >= kMaximumRipples) return;
    ActiveRipple ripple;
    ripple.press_id = event.press_id;
    ripple.origin = OriginFor(node, event);
    ripple.controller.AnimateTo(1.0F, TweenSpec{.duration = style_.duration, .easing = Easing::EaseOut});
    ripples_.push_back(std::move(ripple));
    InvalidatePaint(PaintInvalidation::Foreground);
  }

  void Finish(std::uint64_t press_id) {
    for(auto& ripple : ripples_) {
      if(ripple.press_id == press_id && ripple.controller.IsRunning()) {
        ripple.controller.Set(1.0F);
        InvalidatePaint(PaintInvalidation::Foreground);
      }
    }
  }

  void CancelAll() {
    bool changed = false;
    for(auto& ripple : ripples_) {
      if(ripple.controller.IsRunning()) {
        ripple.controller.Set(1.0F);
        changed = true;
      }
    }
    if(changed) InvalidatePaint(PaintInvalidation::Foreground);
  }

  RippleStyle style_;
  std::vector<ActiveRipple> ripples_;
};

const huxerui::detail::ModifierDescriptor& Ripple::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<Ripple, Ripple::Extension>();
}

Ripple::Ripple(RippleStyle style) {
  ValidateStyle(style);
  this->style = std::move(style);
}

} // namespace huxerui::interaction
