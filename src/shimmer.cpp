#include <huxerui/interaction/shimmer.h>

#include <huxerui/modifier.h>
#include <huxerui/paint.h>

#include <cmath>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const ShimmerStyle& style) {
  if(!std::isfinite(style.period) || style.period <= 0.0) {
    throw std::invalid_argument("HuxerUI Interaction: Shimmer period must be finite and positive");
  }
  if(!std::isfinite(style.band_width) || style.band_width <= 0.0F || style.band_width > 1.0F) {
    throw std::invalid_argument("HuxerUI Interaction: Shimmer band width must be finite, positive, and at most one");
  }
}

} // namespace

/// Retained paint behavior for the Shimmer modifier. The framework preserves this extension across recomposition
/// and calls Update() with the latest modifier value.
class Shimmer::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const Shimmer& value) { Update(node, value); }

  void Update(ViewNode&, const Shimmer& value) {
    style_ = value.style;
    if(!sweep_.IsRunning()) {
      sweep_.Set(0.0F);
      sweep_.AnimateTo(1.0F, TweenSpec{.duration = style_.period, .easing = Easing::Linear},
                       AnimationPlayback{.delay = 0.0, .iterations = std::nullopt, .repeat_mode = RepeatMode::Restart});
    }
    InvalidatePaint(PaintInvalidation::Foreground);
  }

  FrameResult OnFrame(ViewNode&, const FrameInfo& frame) override {
    const MotionAdvanceResult result = sweep_.Advance(frame);
    if(result.changed) InvalidatePaint(PaintInvalidation::Foreground);
    // The sweep mirrors the declarative looping widgets: it rides whatever frame traffic the host already
    // produces instead of requesting frames, so an always-mounted shimmer never keeps the frame loop awake.
    return {false, std::nullopt};
  }

  void PaintAboveContent(const ViewNode& node, PaintContext& context) const override {
    const Rect bounds = node.Bounds();
    if(bounds.IsEmpty() || bounds.width <= 0.0F || bounds.height <= 0.0F) return;
    Color center = style_.color;
    if(center.alpha <= 0.0F) return;
    const float band = style_.band_width;
    const float start = sweep_.Value() * (1.0F + 2.0F * band) - band;
    const float mid = start + band * 0.5F;
    const float end = start + band;
    if(end <= 0.0F || start >= 1.0F) return;
    std::vector<GradientStop> stops;
    stops.push_back({.offset = 0.0F, .color = Color::Transparent()});
    const auto insert = [&stops](float offset, Color color) {
      if(offset > 0.0F && offset < 1.0F) stops.push_back({.offset = offset, .color = color});
    };
    insert(start, Color::Transparent());
    insert(mid, center);
    insert(end, Color::Transparent());
    stops.push_back({.offset = 1.0F, .color = Color::Transparent()});
    context.PushClip(bounds, style_.corner_radii);
    context.DrawRect(bounds,
                     Brush(LinearGradient{.start = {0.0F, 0.0F}, .end = {1.0F, 0.0F}, .stops = std::move(stops)}),
                     style_.corner_radii);
    context.PopClip();
  }

private:
  ShimmerStyle style_;
  MotionController sweep_{1.0F};
};

const detail::ModifierDescriptor& Shimmer::Descriptor() {
  return detail::ModifierDescriptorFor<Shimmer, Shimmer::Extension>();
}

Shimmer::Shimmer(ShimmerStyle style) {
  ValidateStyle(style);
  this->style = std::move(style);
}

} // namespace huxerui::interaction
