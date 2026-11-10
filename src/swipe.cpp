#include <huxerui/interaction/swipe.h>

#include <huxerui/gesture.h>
#include <huxerui/layout.h>
#include <huxerui/modifier.h>
#include <huxerui/state.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const SwipeActionsStyle& style) {
  if(!std::isfinite(style.action_extent) || style.action_extent <= 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: SwipeActions extent must be finite and positive");
  }
  if(!std::isfinite(style.activation_ratio) || style.activation_ratio < 0.0F || style.activation_ratio > 1.0F) {
    throw std::invalid_argument("HuxerUI Interaction: SwipeActions activation ratio must be in [0, 1]");
  }
  if(!std::isfinite(style.minimum_velocity) || style.minimum_velocity < 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: SwipeActions velocity must be finite and non-negative");
  }
}

struct SwipeSession {
  bool active = false;
  float start_offset = 0.0F;

  bool operator==(const SwipeSession&) const = default;
};

/// Lays the action strip behind the content and gives it the content height; clipping and the animated content
/// offset expose only the action extent the user has swiped.
class SwipeActionsLayout final : public Layout<SwipeActionsLayout> {
public:
  using Layout::Layout;

  static LayoutResult Measure(LayoutContext& context, ViewNode& node, Constraints constraints) {
    if(node.ChildCount() < 2) return {};
    ViewNode& actions = node.ChildAt(0);
    ViewNode& content = node.ChildAt(1);
    const Size content_size = context.Measure(content, constraints);
    const Size action_size = context.Measure(actions, constraints.TightHeight(content_size.height));
    const float action_width = std::min(action_size.width, content_size.width);
    LayoutResult result;
    result.Place(actions,
                 {std::max(0.0F, content_size.width - action_width),
                  std::max(0.0F, (content_size.height - action_size.height) * 0.5F)});
    result.Place(content, {});
    return result.SetSize(content_size);
  }
};

} // namespace

View SwipeActions(View content, std::function<View(std::function<void()>)> actions, SwipeActionsStyle style) {
  ValidateStyle(style);
  if(!actions) {
    throw std::invalid_argument("HuxerUI Interaction: SwipeActions requires an action builder");
  }
  return Scope([content = std::move(content), actions = std::move(actions), style]() -> View {
    auto offset = UseState(0.0F);
    auto session = UseState(SwipeSession{});
    const SwipeSession current_session = session.Get();
    const float current_offset = offset.Get();
    const AnimationSpec motion =
        current_session.active ? style.follow_motion : style.settle_motion;

    std::function<void()> close = [offset] { offset = 0.0F; };
    View action_view = actions(std::move(close));
    action_view = std::move(action_view).With(Frame{.width = style.action_extent});
    View content_view = content;
    content_view = std::move(content_view)
                       .With(Offset(AnimateTo(Point{-current_offset, 0.0F}, motion)));

    View layout = SwipeActionsLayout{std::move(action_view), std::move(content_view)};
    return std::move(layout)
        .With(ClipChildren())
        .With(DragGesture{.axis = Axis::Horizontal})
        .On<DragEvents::Started>([session, offset](const DragEvent&) {
          SwipeSession next;
          next.active = true;
          next.start_offset = offset.Get();
          session = next;
        })
        .On<DragEvents::Changed>([session, offset, extent = style.action_extent](const DragEvent& event) {
          const SwipeSession drag = session.Get();
          if(!drag.active) return;
          offset = std::clamp(drag.start_offset - event.translation.x, 0.0F, extent);
        })
        .On<DragEvents::Ended>([session, offset, style](const DragEvent& event) {
          const SwipeSession drag = session.Get();
          if(!drag.active) return;
          const bool open = offset.Get() > style.action_extent * style.activation_ratio
                            || event.velocity.x < -style.minimum_velocity;
          offset = open ? style.action_extent : 0.0F;
          session = SwipeSession{};
        })
        .On<DragEvents::Canceled>([session, offset](const DragEvent&) {
          session = SwipeSession{};
          offset = 0.0F;
        });
  });
}

} // namespace huxerui::interaction
