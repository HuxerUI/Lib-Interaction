#include <huxerui/interaction/visual/expandable.h>

#include "../internal/validate.h"

#include <huxerui/layout.h>
#include <huxerui/modifier.h>
#include <huxerui/state.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const ExpandableStyle& style) {
  if(const auto* tween = std::get_if<TweenSpec>(&style.motion)) {
    internal::RequireNonNegative(tween->duration,
                                 "HuxerUI Interaction: Expandable tween duration must be finite and non-negative");
  } else if(const auto* spring = std::get_if<SpringSpec>(&style.motion)) {
    internal::RequirePositive(spring->stiffness,
                              "HuxerUI Interaction: Expandable spring stiffness must be finite and positive");
    internal::RequireNonNegative(spring->damping_ratio,
                                 "HuxerUI Interaction: Expandable spring damping ratio must be finite and non-negative");
  }
}

struct ExpandLayoutSpec {
  float progress = 1.0F;
  Axis axis = Axis::Vertical;
  bool exclude_collapsed_input = true;

  bool operator==(const ExpandLayoutSpec&) const = default;
};

struct ExpandLayoutKey {
  using Value = ExpandLayoutSpec;
};

class ExpandableLayout final : public Layout<ExpandableLayout> {
public:
  using Layout::Layout;

  static LayoutResult Measure(LayoutContext& context, ViewNode& node, Constraints constraints) {
    if(node.ChildCount() == 0) return {};
    ViewNode& child = node.ChildAt(0);
    const ExpandLayoutSpec spec = node.LayoutValueOr<ExpandLayoutKey>(ExpandLayoutSpec{});
    const float progress = std::clamp(spec.progress, 0.0F, 1.0F);
    const Constraints child_constraints =
        spec.axis == Axis::Vertical ? constraints.LooseHeight() : constraints.LooseWidth();
    const Size child_size = context.Measure(child, child_constraints);
    const Size target_size = spec.axis == Axis::Vertical
                                 ? Size{child_size.width, child_size.height * progress}
                                 : Size{child_size.width * progress, child_size.height};
    LayoutResult result;
    if(progress > 0.0F || !spec.exclude_collapsed_input) {
      result.Place(child, {});
    }
    return result.SetSize(constraints.Constrain(target_size));
  }
};

struct ExpandProgressCell {
  State<float> progress;
};

struct ExpandProgressDriver {
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  std::shared_ptr<ExpandProgressCell> cell;
  float target = 1.0F;
  AnimationSpec motion = TweenSpec{.duration = 0.22, .easing = Easing::EaseInOut};

  bool operator==(const ExpandProgressDriver&) const = default;
};

class ExpandProgressDriver::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const ExpandProgressDriver& value) { Update(node, value); }

  void Update(ViewNode&, const ExpandProgressDriver& value) {
    cell_ = value.cell;
    if(!initialized_) {
      target_ = value.target;
      motion_.Set(target_);
      last_written_ = target_;
      initialized_ = true;
      return;
    }
    if(target_ != value.target) {
      target_ = value.target;
      motion_.AnimateTo(target_, value.motion);
    }
  }

  FrameResult OnFrame(ViewNode&, const FrameInfo& frame) override {
    const MotionAdvanceResult result = motion_.Advance(frame);
    const float clamped = std::clamp(motion_.Value(), 0.0F, 1.0F);
    if(cell_ && cell_->progress.IsValid()) {
      const float next_value = !motion_.IsRunning() ? target_ : clamped;
      if(std::abs(next_value - last_written_) > 1e-6F || (!motion_.IsRunning() && last_written_ != target_)) {
        last_written_ = next_value;
        cell_->progress = next_value;
      }
    }
    return {result.needs_frame, result.wake_after};
  }

private:
  std::shared_ptr<ExpandProgressCell> cell_;
  MotionController motion_{1.0F};
  float target_ = 1.0F;
  float last_written_ = 1.0F;
  bool initialized_ = false;
};

const huxerui::detail::ModifierDescriptor& ExpandProgressDriver::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<ExpandProgressDriver, ExpandProgressDriver::Extension>();
}

} // namespace

View Expandable(View content, bool expanded, ExpandableStyle style) {
  ValidateStyle(style);
  return Scope([content = std::move(content), expanded, style]() -> View {
    const float initial = expanded ? 1.0F : 0.0F;
    auto progress = UseState(initial);
    auto cell = UseState(std::make_shared<ExpandProgressCell>(ExpandProgressCell{progress})).Get();
    cell->progress = progress;
    const float current_progress = std::clamp(progress.Get(), 0.0F, 1.0F);

    View child = content;
    if(style.fade) {
      child = std::move(child).With(Opacity(current_progress));
    }

    View layout = ExpandableLayout{std::move(child)};
    layout = std::move(layout)
                 .LayoutValue<ExpandLayoutKey>(ExpandLayoutSpec{
                     .progress = current_progress,
                     .axis = style.axis,
                     .exclude_collapsed_input = style.exclude_collapsed_input,
                 })
                 .With(ClipChildren())
                 .With(ExpandProgressDriver{
                     .cell = std::move(cell),
                     .target = expanded ? 1.0F : 0.0F,
                     .motion = style.motion,
                 });
    return layout;
  });
}

} // namespace huxerui::interaction
