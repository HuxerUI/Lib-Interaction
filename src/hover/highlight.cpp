#include <huxerui/interaction/hover/highlight.h>

#include "../internal/motion_group.h"

#include <huxerui/modifier.h>
#include <huxerui/paint.h>

#include <algorithm>
#include <utility>

namespace huxerui::interaction {

class HoverHighlight::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const HoverHighlight& value) { Update(node, value); }

  void Update(ViewNode&, const HoverHighlight& value) {
    style_ = value.style;
    InvalidatePaint(PaintInvalidation::Foreground);
  }

  [[nodiscard]] bool HitTest(ViewNode& node, Point position) const override {
    return node.Bounds().Contains(position);
  }

  [[nodiscard]] bool HoverHitTest(ViewNode& node, Point position) const override {
    return node.Bounds().Contains(position);
  }

  void OnInteraction(ViewNode&, const InteractionState& state, const std::optional<InteractionEvent>&) override {
    DriveFrom(state);
  }

  FrameResult OnFrame(ViewNode&, const FrameInfo& frame) override {
    internal::FrameAccumulator acc;
    acc.Add(presence_.Advance(frame));
    acc.Add(focus_.Advance(frame));
    if(acc.changed) {
      InvalidatePaint(PaintInvalidation::Foreground);
    }
    return acc.Result();
  }

  void PaintAboveContent(const ViewNode& node, PaintContext& context) const override {
    const Rect bounds = node.Bounds();
    if(bounds.IsEmpty()) return;
    const float focus = std::clamp(focus_.Value(), 0.0F, 1.0F);
    if(focus > 0.0F && style_.focus_color.alpha > 0.0F) {
      Color overlay = style_.focus_color;
      overlay.alpha *= focus;
      context.PushClip(bounds, style_.corner_radii);
      context.DrawRect(bounds, overlay, style_.corner_radii);
      context.PopClip();
    }
    const float presence = std::clamp(presence_.Value(), 0.0F, 2.0F);
    if(presence <= 0.0F) return;
    Color overlay;
    if(presence <= 1.0F) {
      overlay = Blend(Color::Transparent(), style_.hover_color, presence);
    } else {
      overlay = Blend(style_.hover_color, style_.pressed_color, presence - 1.0F);
    }
    if(overlay.alpha <= 0.0F) return;
    context.PushClip(bounds, style_.corner_radii);
    context.DrawRect(bounds, overlay, style_.corner_radii);
    context.PopClip();
  }

private:
  static Color Blend(Color from, Color to, float progress) {
    return {from.red + (to.red - from.red) * progress, from.green + (to.green - from.green) * progress,
            from.blue + (to.blue - from.blue) * progress, from.alpha + (to.alpha - from.alpha) * progress};
  }

  void DriveFrom(const InteractionState& state) {
    const float presence_target =
        state.enabled ? (state.pressed ? 2.0F : (state.hovered ? 1.0F : 0.0F)) : 0.0F;
    if(presence_.Target() != presence_target) {
      presence_.AnimateTo(presence_target, style_.motion);
      InvalidatePaint(PaintInvalidation::Foreground);
    }
    const float focus_target = state.enabled && state.focus_visible ? 1.0F : 0.0F;
    if(focus_.Target() != focus_target) {
      focus_.AnimateTo(focus_target, style_.motion);
      InvalidatePaint(PaintInvalidation::Foreground);
    }
  }

  HoverHighlightStyle style_;
  MotionController presence_{0.0F};
  MotionController focus_{0.0F};
};

const huxerui::detail::ModifierDescriptor& HoverHighlight::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<HoverHighlight, HoverHighlight::Extension>();
}

HoverHighlight::HoverHighlight(HoverHighlightStyle style) : style(std::move(style)) {}

} // namespace huxerui::interaction
