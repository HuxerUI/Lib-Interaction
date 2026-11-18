#include <huxerui/interaction/gesture/swipe.h>

#include "../internal/validate.h"

#include <huxerui/gesture.h>
#include <huxerui/layout.h>
#include <huxerui/modifier.h>
#include <huxerui/state.h>

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {

namespace detail {
struct SwipeControllerState {
  State<float> offset;
  State<bool> open;
  float action_extent = 88.0F;
  std::function<void(bool)> set_open_fn;
};
} // namespace detail

namespace {

void ValidateStyle(const SwipeActionsStyle& style) {
  internal::RequirePositive(style.action_extent,
                            "HuxerUI Interaction: SwipeActions extent must be finite and positive");
  internal::RequireUnitInterval(style.activation_ratio,
                                "HuxerUI Interaction: SwipeActions activation ratio must be in [0, 1]");
  internal::RequireNonNegative(
      style.minimum_velocity, "HuxerUI Interaction: SwipeActions velocity must be finite and non-negative");
}

struct SwipeSession {
  bool active = false;
  float start_offset = 0.0F;

  bool operator==(const SwipeSession&) const = default;
};

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

bool SwipeController::IsOpen() const {
  if(!state_ || !state_->open.IsValid()) return false;
  return state_->open.Get();
}

float SwipeController::Offset() const {
  if(!state_ || !state_->offset.IsValid()) return 0.0F;
  return state_->offset.Get();
}

float SwipeController::Progress() const {
  if(!state_ || !state_->offset.IsValid() || state_->action_extent <= 0.0F) return 0.0F;
  return std::clamp(state_->offset.Get() / state_->action_extent, 0.0F, 1.0F);
}

void SwipeController::Open() const {
  if(!state_) return;
  if(state_->set_open_fn) {
    state_->set_open_fn(true);
  } else {
    if(state_->open.IsValid()) state_->open = true;
    if(state_->offset.IsValid()) state_->offset = state_->action_extent;
  }
}

void SwipeController::Close() const {
  if(!state_) return;
  if(state_->set_open_fn) {
    state_->set_open_fn(false);
  } else {
    if(state_->open.IsValid()) state_->open = false;
    if(state_->offset.IsValid()) state_->offset = 0.0F;
  }
}

void SwipeController::Toggle() const {
  if(IsOpen()) {
    Close();
  } else {
    Open();
  }
}

SwipeController UseSwipeController(bool initially_open) {
  auto offset = UseState(initially_open ? 88.0F : 0.0F);
  auto open = UseState(initially_open);
  auto holder = UseState(std::make_shared<detail::SwipeControllerState>());
  auto shared = holder.Get();
  shared->offset = offset;
  shared->open = open;
  return SwipeController(std::move(shared));
}

View SwipeActions(View content, std::function<View(SwipeActionContext)> actions, SwipeActionsStyle style,
                  std::function<void(bool)> on_open_changed) {
  ValidateStyle(style);
  if(!actions) {
    throw std::invalid_argument("HuxerUI Interaction: SwipeActions requires an action builder");
  }
  return Scope([content = std::move(content), actions = std::move(actions), style,
                on_open_changed = std::move(on_open_changed)]() -> View {
    auto local_offset = UseState(0.0F);
    auto local_open = UseState(false);
    auto session = UseState(SwipeSession{});

    const bool has_controller = style.controller.state_ && style.controller.state_->offset.IsValid();
    const State<float> offset = has_controller ? style.controller.state_->offset : local_offset;
    const State<bool> open_state = has_controller ? style.controller.state_->open : local_open;

    auto apply_open = [offset, open_state, extent = style.action_extent, on_open_changed](bool next_open) {
      const bool prev = open_state.Get();
      offset = next_open ? extent : 0.0F;
      if(prev != next_open) {
        open_state = next_open;
        if(on_open_changed) on_open_changed(next_open);
      }
    };

    if(style.controller.state_) {
      style.controller.state_->action_extent = style.action_extent;
      style.controller.state_->set_open_fn = apply_open;
    }

    const SwipeSession current_session = session.Get();
    const float current_offset = !current_session.active && open_state.Get()
                                     ? style.action_extent
                                     : std::clamp(offset.Get(), 0.0F, style.action_extent);
    const float progress = std::clamp(current_offset / style.action_extent, 0.0F, 1.0F);
    const AnimationSpec motion = current_session.active ? style.follow_motion : style.settle_motion;

    std::function<void()> close = [session, apply_open] {
      session = SwipeSession{};
      apply_open(false);
    };
    View action_view = actions(SwipeActionContext{.close = std::move(close), .progress = progress});
    action_view = std::move(action_view).With(Frame{.width = style.action_extent});
    View content_view = content;
    content_view = std::move(content_view).With(Offset(AnimateTo(Point{-current_offset, 0.0F}, motion)));

    View layout = SwipeActionsLayout{std::move(action_view), std::move(content_view)};
    return std::move(layout)
        .With(ClipChildren())
        .With(DragGesture{.axis = Axis::Horizontal})
        .On<DragEvents::Started>([session, current_offset](const DragEvent&) {
          session = SwipeSession{.active = true, .start_offset = current_offset};
        })
        .On<DragEvents::Changed>([session, offset, extent = style.action_extent](const DragEvent& event) {
          const SwipeSession drag = session.Get();
          if(!drag.active) return;
          offset = std::clamp(drag.start_offset - event.translation.x, 0.0F, extent);
        })
        .On<DragEvents::Ended>([session, offset, style, apply_open](const DragEvent& event) {
          const SwipeSession drag = session.Get();
          if(!drag.active) return;
          const bool next_open = offset.Get() > style.action_extent * style.activation_ratio
                                 || event.velocity.x < -style.minimum_velocity;
          session = SwipeSession{};
          apply_open(next_open);
        })
        .On<DragEvents::Canceled>([session, apply_open](const DragEvent&) {
          session = SwipeSession{};
          apply_open(false);
        });
  });
}

} // namespace huxerui::interaction
