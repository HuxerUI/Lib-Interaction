#include <huxerui/interaction/longpress.h>

#include <huxerui/modifier.h>
#include <huxerui/paint.h>
#include <huxerui/state.h>

#include <chrono>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {
namespace {

// Matches GestureSettings::long_press_duration's default; used when the style leaves the duration unset.
constexpr std::chrono::duration<double> kDefaultHoldDuration{0.5};

void ValidateStyle(const LongPressableStyle& style) {
  if(style.minimum_duration.has_value() && style.minimum_duration->count() <= 0.0) {
    throw std::invalid_argument("HuxerUI Interaction: LongPressable duration must be positive");
  }
  if(style.maximum_movement.has_value()
     && (!std::isfinite(*style.maximum_movement) || *style.maximum_movement <= 0.0F)) {
    throw std::invalid_argument("HuxerUI Interaction: LongPressable movement tolerance must be finite and positive");
  }
  if(!std::isfinite(style.held_scale) || style.held_scale <= 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: LongPressable held scale must be finite and positive");
  }
}

/// Circular hold progress ring painted above the content. The extension observes the pointer sequence without
/// consuming it and advances one MotionController across the hold duration.
struct HoldProgress {
  /// Retained extension created by the framework when the modifier is attached through With().
  class Extension;

  static const detail::ModifierDescriptor& Descriptor();

  Color color = Color::Rgb(255, 255, 255, 0.9F);
  double duration = 0.5;

  bool operator==(const HoldProgress&) const = default;
};

class HoldProgress::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const HoldProgress& value) { Update(node, value); }

  void Update(ViewNode&, const HoldProgress& value) {
    style_ = value;
    InvalidatePaint(PaintInvalidation::Foreground);
  }

  /// Lets the extension observe pointer sequences routed over the node without consuming them.
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
    const float radius = diameter * 0.5F - 8.0F;
    if(radius <= 0.0F) return;
    const Point center{bounds.x + bounds.width * 0.5F, bounds.y + bounds.height * 0.5F};
    const StrokeStyle ring_stroke{.width = 3.0F, .cap = StrokeCap::Round};
    Color track = style_.color;
    track.alpha *= 0.2F;
    context.DrawArc(center, radius, 0.0F, 3.14159265F, track, ring_stroke);
    context.DrawArc(center, radius, 3.14159265F, 3.14159265F, track, ring_stroke);
    if(progress < 1.0F) {
      context.DrawArc(center, radius, -1.57079637F, progress * 6.28318548F, style_.color, ring_stroke);
    }
  }

private:
  HoldProgress style_;
  MotionController ring_{0.0F};
  std::optional<std::int64_t> pointer_;
};

const detail::ModifierDescriptor& HoldProgress::Descriptor() {
  return detail::ModifierDescriptorFor<HoldProgress, HoldProgress::Extension>();
}

} // namespace

View LongPressable(View content, LongPressableStyle style, std::function<void()> on_long_press) {
  ValidateStyle(style);
  const std::chrono::duration<double> hold_duration =
      style.minimum_duration.value_or(kDefaultHoldDuration);
  return Scope([content = std::move(content), style, on_long_press = std::move(on_long_press),
                hold_duration]() -> View {
    auto holding = UseState(false);
    std::optional<std::chrono::duration<double>> minimum_duration = style.minimum_duration;
    if(style.show_progress && !minimum_duration.has_value()) {
      // Pin the gesture to the ring's duration so recognition and the visual progress stay synchronized.
      minimum_duration = hold_duration;
    }
    View interactive = content;
    interactive = std::move(interactive)
                      .With(Scale(AnimateTo(holding ? style.held_scale : 1.0F, style.animation), style.origin))
                      .With(LongPressGesture{
                          .minimum_duration = minimum_duration,
                          .maximum_movement = style.maximum_movement,
                      })
                      .On<LongPressEvents::Started>([holding, on_long_press](const LongPressEvent&) {
                        holding = true;
                        if(on_long_press) on_long_press();
                      })
                      .On<LongPressEvents::Ended>([holding](const LongPressEvent&) { holding = false; })
                      .On<LongPressEvents::Canceled>([holding](const LongPressEvent&) { holding = false; });
    if(style.show_progress) {
      interactive = std::move(interactive).With(
          HoldProgress{.color = style.progress_color, .duration = hold_duration.count()});
    }
    if(style.cursor != PointerCursorKind::Default) {
      interactive = std::move(interactive).With(PointerCursor{style.cursor});
    }
    return interactive;
  });
}

} // namespace huxerui::interaction
