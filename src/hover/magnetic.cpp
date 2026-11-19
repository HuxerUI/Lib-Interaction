#include <huxerui/interaction/hover/magnetic.h>

#include "../internal/validate.h"

#include <huxerui/layout.h>
#include <huxerui/modifier.h>
#include <huxerui/state.h>

#include <algorithm>
#include <memory>
#include <utility>
#include <variant>

namespace huxerui::interaction {
namespace {

void ValidateMotion(const AnimationSpec& spec, const char* tween_message, const char* stiffness_message,
                    const char* damping_message) {
  if(const auto* tween = std::get_if<TweenSpec>(&spec)) {
    internal::RequireNonNegative(tween->duration, tween_message);
  } else if(const auto* spring = std::get_if<SpringSpec>(&spec)) {
    internal::RequirePositive(spring->stiffness, stiffness_message);
    internal::RequireNonNegative(spring->damping_ratio, damping_message);
  }
}

void ValidateStyle(const MagneticStyle& style) {
  internal::RequireNonNegative(style.maximum_offset.x,
                               "HuxerUI Interaction: Magnetic maximum offset x must be finite and non-negative");
  internal::RequireNonNegative(style.maximum_offset.y,
                               "HuxerUI Interaction: Magnetic maximum offset y must be finite and non-negative");
  internal::RequirePositive(style.hovered_scale, "HuxerUI Interaction: Magnetic scale must be finite and positive");
  internal::RequireFinite(style.origin.x, "HuxerUI Interaction: Magnetic origin must be finite");
  internal::RequireFinite(style.origin.y, "HuxerUI Interaction: Magnetic origin must be finite");
  ValidateMotion(style.follow_motion, "HuxerUI Interaction: Magnetic follow duration must be finite and non-negative",
                 "HuxerUI Interaction: Magnetic follow spring stiffness must be finite and positive",
                 "HuxerUI Interaction: Magnetic follow spring damping ratio must be finite and non-negative");
  ValidateMotion(style.return_motion, "HuxerUI Interaction: Magnetic return duration must be finite and non-negative",
                 "HuxerUI Interaction: Magnetic return spring stiffness must be finite and positive",
                 "HuxerUI Interaction: Magnetic return spring damping ratio must be finite and non-negative");
}

struct MagneticCell {
  State<bool> hovered;
  State<Point> offset;
};

struct MagneticProbe {
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  std::shared_ptr<MagneticCell> cell;
  Point maximum_offset;

  bool operator==(const MagneticProbe&) const = default;
};

class MagneticProbe::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const MagneticProbe& value) { Update(node, value); }

  void Update(ViewNode&, const MagneticProbe& value) {
    cell_ = value.cell;
    maximum_offset_ = value.maximum_offset;
  }

  [[nodiscard]] bool HoverHitTest(ViewNode& node, Point position) const override {
    return node.Bounds().Contains(position);
  }

  void OnHover(ViewNode& node, const HoverEvent& event) override {
    if(!cell_) return;
    if(event.type == HoverEventType::Leave) {
      if(cell_->hovered.IsValid() && cell_->hovered.Get()) {
        cell_->hovered = false;
      }
      if(cell_->offset.IsValid() && cell_->offset.Get() != Point{}) {
        cell_->offset = Point{};
      }
      return;
    }

    const Rect bounds = node.Bounds();
    Point target_offset{};
    if(bounds.width > 0.0F) {
      const float half_width = bounds.width * 0.5F;
      const float nx = std::clamp((event.position.x - (bounds.x + half_width)) / half_width, -1.0F, 1.0F);
      target_offset.x = nx * maximum_offset_.x;
    }
    if(bounds.height > 0.0F) {
      const float half_height = bounds.height * 0.5F;
      const float ny = std::clamp((event.position.y - (bounds.y + half_height)) / half_height, -1.0F, 1.0F);
      target_offset.y = ny * maximum_offset_.y;
    }
    if(cell_->hovered.IsValid() && !cell_->hovered.Get()) {
      cell_->hovered = true;
    }
    if(cell_->offset.IsValid() && cell_->offset.Get() != target_offset) {
      cell_->offset = target_offset;
    }
  }

private:
  std::shared_ptr<MagneticCell> cell_;
  Point maximum_offset_;
};

const huxerui::detail::ModifierDescriptor& MagneticProbe::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<MagneticProbe, MagneticProbe::Extension>();
}

} // namespace

View Magnetic(View content, MagneticStyle style) {
  ValidateStyle(style);
  return Scope([content = std::move(content), style]() -> View {
    auto hovered = UseState(false);
    auto offset = UseState(Point{});
    auto cell = UseState(std::make_shared<MagneticCell>(MagneticCell{hovered, offset})).Get();
    cell->hovered = hovered;
    cell->offset = offset;

    const bool is_hovered = hovered.Get();
    const Point current_offset = offset.Get();
    const AnimationSpec& motion = is_hovered ? style.follow_motion : style.return_motion;

    View inner = content;
    inner = std::move(inner)
                .With(Offset(AnimateTo(current_offset, motion)))
                .With(Scale(AnimateTo(is_hovered ? style.hovered_scale : 1.0F, motion), style.origin));

    View outer = Stack{std::move(inner)}.With(MagneticProbe{
        .cell = std::move(cell),
        .maximum_offset = style.maximum_offset,
    });
    if(style.cursor != PointerCursorKind::Default) {
      outer = std::move(outer).With(PointerCursor{style.cursor});
    }
    return outer;
  });
}

} // namespace huxerui::interaction
