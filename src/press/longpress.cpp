#include <huxerui/interaction/press/longpress.h>

#include "../internal/validate.h"

#include <huxerui/modifier.h>
#include <huxerui/paint.h>
#include <huxerui/semantics.h>
#include <huxerui/state.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <optional>
#include <utility>

namespace huxerui::interaction {
namespace {

constexpr std::chrono::duration<double> kDefaultHoldDuration{0.5};

void ValidateHoldProgressStyle(const HoldProgressStyle& style) {
  internal::RequirePositive(style.duration, "HuxerUI Interaction: HoldProgressRing duration must be finite and positive");
  internal::RequirePositive(style.stroke_width,
                            "HuxerUI Interaction: HoldProgressRing stroke width must be finite and positive");
  internal::RequireNonNegative(style.inset,
                               "HuxerUI Interaction: HoldProgressRing inset must be finite and non-negative");
}

void ValidateStyle(const LongPressableStyle& style) {
  internal::RequireOptionalPositiveDuration(style.minimum_duration,
                                            "HuxerUI Interaction: LongPressable duration must be finite and positive");
  internal::RequireOptionalPositive(
      style.maximum_movement, "HuxerUI Interaction: LongPressable movement tolerance must be finite and positive");
  internal::RequirePositive(style.held_scale,
                            "HuxerUI Interaction: LongPressable held scale must be finite and positive");
  internal::RequirePositive(style.progress_stroke_width,
                            "HuxerUI Interaction: LongPressable progress stroke width must be finite and positive");
  internal::RequireNonNegative(style.progress_inset,
                               "HuxerUI Interaction: LongPressable progress inset must be finite and non-negative");
}

} // namespace

class HoldProgressRing::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const HoldProgressRing& value) { Update(node, value); }

  void Update(ViewNode&, const HoldProgressRing& value) {
    style_ = value.style;
    InvalidatePaint(PaintInvalidation::Foreground);
  }

  [[nodiscard]] bool HitTest(ViewNode& node, Point position) const override {
    return node.Bounds().Contains(position);
  }

  PointerResult OnPointer(ViewNode&, const PointerEvent& event) override {
    if(event.type == PointerEventType::Down && event.changed_button == PointerButton::Primary) {
      pointer_ = event.pointer_id;
      ring_.Set(0.0F);
      ring_.AnimateTo(1.0F, TweenSpec{.duration = style_.duration, .easing = Easing::Linear});
      InvalidatePaint(PaintInvalidation::Foreground);
      return PointerResult::Observe;
    }
    if(!pointer_.has_value() || event.pointer_id != *pointer_) return PointerResult::Ignored;
    if(event.type == PointerEventType::Up || event.type == PointerEventType::Cancel) {
      pointer_ = std::nullopt;
      if(ring_.IsRunning()) {
        ring_.AnimateTo(0.0F, TweenSpec{.duration = 0.1, .easing = Easing::Linear});
        InvalidatePaint(PaintInvalidation::Foreground);
      }
    }
    return PointerResult::Ignored;
  }

  FrameResult OnFrame(ViewNode&, const FrameInfo& frame) override {
    const MotionAdvanceResult result = ring_.Advance(frame);
    if(result.changed) InvalidatePaint(PaintInvalidation::Foreground);
    return {result.needs_frame, result.wake_after};
  }

  void PaintAboveContent(const ViewNode& node, PaintContext& context) const override {
    const float progress = ring_.Value();
    if(progress <= 0.0F) return;
    const Rect bounds = node.Bounds();
    if(bounds.IsEmpty() || bounds.width <= 0.0F || bounds.height <= 0.0F) return;
    const float diameter = std::min(bounds.width, bounds.height);
    const float radius = diameter * 0.5F - style_.inset;
    if(radius <= 0.0F) return;
    const Point center{bounds.x + bounds.width * 0.5F, bounds.y + bounds.height * 0.5F};
    const StrokeStyle ring_stroke{.width = style_.stroke_width, .cap = StrokeCap::Round};
    Color track = style_.color;
    track.alpha *= 0.2F;
    context.DrawArc(center, radius, 0.0F, 3.14159265F, track, ring_stroke);
    context.DrawArc(center, radius, 3.14159265F, 3.14159265F, track, ring_stroke);
    if(progress < 1.0F) {
      context.DrawArc(center, radius, -1.57079637F, progress * 6.28318548F, style_.color, ring_stroke);
    }
  }

private:
  HoldProgressStyle style_;
  MotionController ring_{0.0F};
  std::optional<std::int64_t> pointer_;
};

const huxerui::detail::ModifierDescriptor& HoldProgressRing::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<HoldProgressRing, HoldProgressRing::Extension>();
}

HoldProgressRing::HoldProgressRing(HoldProgressStyle style) {
  ValidateHoldProgressStyle(style);
  this->style = std::move(style);
}

namespace detail {

View LongPressableImpl(View content, std::function<void()> on_long_press, LongPressableStyle style,
                       std::function<void(bool)> on_holding_changed) {
  ValidateStyle(style);
  const std::chrono::duration<double> hold_duration =
      style.minimum_duration.value_or(kDefaultHoldDuration);
  return Scope([content = std::move(content), style, on_long_press = std::move(on_long_press),
                on_holding_changed = std::move(on_holding_changed), hold_duration]() -> View {
    auto holding = UseState(false);
    std::optional<std::chrono::duration<double>> minimum_duration = style.minimum_duration;
    if(style.show_progress && !minimum_duration.has_value()) {
      minimum_duration = hold_duration;
    }
    View interactive = content;
    interactive = std::move(interactive)
                      .With(Scale(AnimateTo(holding.Get() ? style.held_scale : 1.0F, style.animation), style.origin))
                      .With(LongPressGesture{
                          .minimum_duration = minimum_duration,
                          .maximum_movement = style.maximum_movement,
                      })
                      .On<LongPressEvents::Started>([holding, on_long_press, on_holding_changed](const LongPressEvent&) {
                        holding = true;
                        if(on_holding_changed) on_holding_changed(true);
                        if(on_long_press) on_long_press();
                      })
                      .On<LongPressEvents::Ended>([holding, on_holding_changed](const LongPressEvent&) {
                        if(holding.Get() && on_holding_changed) on_holding_changed(false);
                        holding = false;
                      })
                      .On<LongPressEvents::Canceled>([holding, on_holding_changed](const LongPressEvent&) {
                        if(holding.Get() && on_holding_changed) on_holding_changed(false);
                        holding = false;
                      });
    if(style.show_progress) {
      interactive = std::move(interactive).With(HoldProgressRing{HoldProgressStyle{
          .color = style.progress_color,
          .duration = hold_duration.count(),
          .stroke_width = style.progress_stroke_width,
          .inset = style.progress_inset,
      }});
    }
    if(on_long_press) {
      interactive = std::move(interactive).With(Semantics{.role = SemanticRole::Button});
    }
    if(style.cursor != PointerCursorKind::Default) {
      interactive = std::move(interactive).With(PointerCursor{style.cursor});
    }
    return interactive;
  });
}

} // namespace detail
} // namespace huxerui::interaction
