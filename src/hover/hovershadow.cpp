#include <huxerui/interaction/hover/hovershadow.h>

#include "../internal/motion_group.h"
#include "../internal/validate.h"

#include <huxerui/animation.h>
#include <huxerui/event.h>
#include <huxerui/modifier.h>
#include <huxerui/paint.h>
#include <huxerui/vector.h>

#include <algorithm>
#include <utility>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const HoverShadowStyle& style) {
  internal::RequireFinite(style.rest_offset, "HuxerUI Interaction: HoverShadow rest offset must be finite");
  internal::RequireNonNegative(style.rest_blur,
                               "HuxerUI Interaction: HoverShadow rest blur must be finite and non-negative");
  internal::RequireFinite(style.rest_spread, "HuxerUI Interaction: HoverShadow rest spread must be finite");
  internal::RequireFinite(style.hover_offset, "HuxerUI Interaction: HoverShadow hover offset must be finite");
  internal::RequireNonNegative(style.max_pointer_offset.x,
                               "HuxerUI Interaction: HoverShadow max pointer offset x must be finite and non-negative");
  internal::RequireNonNegative(style.max_pointer_offset.y,
                               "HuxerUI Interaction: HoverShadow max pointer offset y must be finite and non-negative");
  internal::RequireNonNegative(style.blur_radius,
                               "HuxerUI Interaction: HoverShadow blur radius must be finite and non-negative");
  internal::RequireFinite(style.spread, "HuxerUI Interaction: HoverShadow spread must be finite");
  internal::RequireNonNegative(
      style.border_glow_width,
      "HuxerUI Interaction: HoverShadow border glow width must be finite and non-negative");
  internal::RequirePositive(
      style.border_glow_diameter,
      "HuxerUI Interaction: HoverShadow border glow diameter must be finite and positive");
  internal::RequireNonNegative(style.fade, "HuxerUI Interaction: HoverShadow fade must be finite and non-negative");
  internal::RequireNonNegative(style.corner_radii.top_left,
                               "HuxerUI Interaction: HoverShadow corner radii must be finite and non-negative");
  internal::RequireNonNegative(style.corner_radii.top_right,
                               "HuxerUI Interaction: HoverShadow corner radii must be finite and non-negative");
  internal::RequireNonNegative(style.corner_radii.bottom_right,
                               "HuxerUI Interaction: HoverShadow corner radii must be finite and non-negative");
  internal::RequireNonNegative(style.corner_radii.bottom_left,
                               "HuxerUI Interaction: HoverShadow corner radii must be finite and non-negative");
}

float Lerp(float from, float to, float t) noexcept {
  return from + (to - from) * t;
}

struct HoverShadowLayer {
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  HoverShadowStyle style{};

  bool operator==(const HoverShadowLayer&) const = default;
};

class HoverShadowLayer::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const HoverShadowLayer& value) { Update(node, value); }

  void Update(ViewNode&, const HoverShadowLayer& value) {
    style_ = value.style;
    Invalidate();
  }

  [[nodiscard]] bool HoverHitTest(ViewNode& node, Point position) const override {
    return node.Bounds().Contains(position);
  }

  void OnHover(ViewNode& node, const HoverEvent& event) override {
    const TweenSpec presence_tween{.duration = style_.fade, .easing = Easing::EaseOut};
    if(event.type == HoverEventType::Leave) {
      presence_.AnimateTo(0.0F, presence_tween);
      dir_x_.AnimateTo(0.0F, presence_tween);
      dir_y_.AnimateTo(0.0F, presence_tween);
      Invalidate();
      return;
    }

    pointer_position_ = event.position;
    has_pointer_ = true;

    const Rect bounds = node.Bounds();
    float nx = 0.0F;
    float ny = 0.0F;
    if(bounds.width > 0.0F) {
      const float half_w = bounds.width * 0.5F;
      nx = std::clamp((event.position.x - (bounds.x + half_w)) / half_w, -1.0F, 1.0F);
    }
    if(bounds.height > 0.0F) {
      const float half_h = bounds.height * 0.5F;
      ny = std::clamp((event.position.y - (bounds.y + half_h)) / half_h, -1.0F, 1.0F);
    }
    const TweenSpec follow_tween{.duration = std::min(style_.fade, 0.12), .easing = Easing::EaseOut};
    dir_x_.AnimateTo(nx, follow_tween);
    dir_y_.AnimateTo(ny, follow_tween);
    if(presence_.Target() != 1.0F) {
      presence_.AnimateTo(1.0F, presence_tween);
    }
    Invalidate();
  }

  FrameResult OnFrame(ViewNode&, const FrameInfo& frame) override {
    internal::FrameAccumulator acc;
    acc.Add(presence_.Advance(frame));
    acc.Add(dir_x_.Advance(frame));
    acc.Add(dir_y_.Advance(frame));
    if(presence_.Value() <= 0.0F && !presence_.IsRunning()) {
      has_pointer_ = false;
    }
    if(acc.changed) Invalidate();
    return acc.Result();
  }

  void PaintBehindContent(const ViewNode& node, PaintContext& context) const override {
    const Rect bounds = node.Bounds();
    if(bounds.IsEmpty() || bounds.width <= 0.0F || bounds.height <= 0.0F) return;
    const float p = std::clamp(presence_.Value(), 0.0F, 1.0F);

    if(style_.rest_color.alpha > 0.0F && p < 1.0F) {
      Color rest = style_.rest_color;
      rest.alpha *= (1.0F - p);
      if(rest.alpha > 0.0F) {
        context.DrawShadow(bounds, rest, style_.rest_offset, style_.rest_blur, style_.rest_spread, style_.corner_radii);
      }
    }

    if(style_.color.alpha > 0.0F && p > 0.0F) {
      Color hover = style_.color;
      hover.alpha *= p;
      if(hover.alpha > 0.0F) {
        const Point offset{
            Lerp(style_.rest_offset.x, style_.hover_offset.x, p) + dir_x_.Value() * style_.max_pointer_offset.x,
            Lerp(style_.rest_offset.y, style_.hover_offset.y, p) + dir_y_.Value() * style_.max_pointer_offset.y,
        };
        const float blur = std::max(0.0F, Lerp(style_.rest_blur, style_.blur_radius, p));
        const float spread = Lerp(style_.rest_spread, style_.spread, p);
        context.DrawShadow(bounds, hover, offset, blur, spread, style_.corner_radii);
      }
    }
  }

  void PaintAboveContent(const ViewNode& node, PaintContext& context) const override {
    if(!has_pointer_ || !HasBorderGlow()) return;
    const float p = std::clamp(presence_.Value(), 0.0F, 1.0F);
    if(p <= 0.0F) return;

    const Rect bounds = node.Bounds();
    if(bounds.IsEmpty() || bounds.width <= 0.0F || bounds.height <= 0.0F) return;

    const float half = style_.border_glow_width * 0.5F;
    const Rect stroke_rect{
        bounds.x + half,
        bounds.y + half,
        bounds.width - style_.border_glow_width,
        bounds.height - style_.border_glow_width,
    };
    if(stroke_rect.width <= 0.0F || stroke_rect.height <= 0.0F) return;

    const CornerRadii inset_radii{
        std::max(0.0F, style_.corner_radii.top_left - half),
        std::max(0.0F, style_.corner_radii.top_right - half),
        std::max(0.0F, style_.corner_radii.bottom_right - half),
        std::max(0.0F, style_.corner_radii.bottom_left - half),
    };
    Path perimeter = Path::RoundedRect(stroke_rect, inset_radii);
    if(perimeter.IsEmpty()) return;

    Color rim_color = style_.border_glow_color;
    rim_color.alpha *= p;
    const float radius_px = style_.border_glow_diameter * 0.5F;
    const RadialGradient rim{
        .center = {(pointer_position_.x - bounds.x) / bounds.width,
                   (pointer_position_.y - bounds.y) / bounds.height},
        .radius = {radius_px / bounds.width, radius_px / bounds.height},
        .stops = {{.offset = 0.0F, .color = rim_color}, {.offset = 1.0F, .color = Color::Transparent()}},
    };
    context.StrokePath(std::move(perimeter), Brush(rim), bounds, StrokeStyle{.width = style_.border_glow_width});
  }

private:
  [[nodiscard]] bool HasBorderGlow() const noexcept {
    return style_.border_glow_width > 0.0F && style_.border_glow_color.alpha > 0.0F;
  }

  void Invalidate() {
    InvalidatePaint(HasBorderGlow() ? PaintInvalidation::Both : PaintInvalidation::Content);
  }

  HoverShadowStyle style_;
  MotionController presence_{0.0F};
  MotionController dir_x_{0.0F};
  MotionController dir_y_{0.0F};
  Point pointer_position_{};
  bool has_pointer_ = false;
};

const huxerui::detail::ModifierDescriptor& HoverShadowLayer::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<HoverShadowLayer, HoverShadowLayer::Extension>();
}

} // namespace

View HoverShadow(View content, HoverShadowStyle style) {
  ValidateStyle(style);
  return Stack{std::move(content)}.With(HoverShadowLayer{.style = std::move(style)});
}

} // namespace huxerui::interaction
