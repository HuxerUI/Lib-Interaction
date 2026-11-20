#include <huxerui/interaction/scroll/scrolltotop.h>

#include "../internal/scroll_animator.h"
#include "../internal/validate.h"

#include <huxerui/environment.h>
#include <huxerui/layout.h>
#include <huxerui/lifecycle.h>
#include <huxerui/modifier.h>
#include <huxerui/state.h>
#include <huxerui/task.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

namespace huxerui::interaction {

namespace detail {

struct ScrollToTopControllerState {
  State<bool> visible;
  State<bool> scrolling_to_top;
  State<float> offset;
  State<float> progress;
  ScrollToTopStyle style;
  std::function<void()> on_triggered;

  bool has_last_offset = false;
  float last_offset = 0.0F;
  float peak_offset = 0.0F;
  float trough_offset = 0.0F;
  bool up_scroll_visible = false;
  bool animating = false;

  bool sync_scheduled = false;
  bool needs_sync = false;
  std::shared_ptr<bool> alive;
  std::function<void()> post_sync;
  std::function<void(float from, float to, const AnimationSpec& motion)> start_smooth_scroll;
  std::function<void()> cancel_smooth_scroll;
};

} // namespace detail

namespace {

constexpr float kOffsetEpsilon = 0.25F;
constexpr float kMoveEpsilon = 0.5F;
constexpr float kProgressEpsilon = 0.002F;
constexpr float kInterruptEpsilon = 1.0F;

void ValidateStyle(const ScrollToTopStyle& style) {
  internal::RequireNonNegative(style.threshold,
                               "HuxerUI Interaction: ScrollToTop threshold must be finite and non-negative");
  internal::RequireNonNegative(style.target_offset,
                               "HuxerUI Interaction: ScrollToTop target offset must be finite and non-negative");
  internal::RequireNonNegative(style.scroll_up_delta,
                               "HuxerUI Interaction: ScrollToTop scroll-up delta must be finite and non-negative");
  internal::RequireNonNegative(style.margin.top,
                               "HuxerUI Interaction: ScrollToTop margin top must be finite and non-negative");
  internal::RequireNonNegative(style.margin.right,
                               "HuxerUI Interaction: ScrollToTop margin right must be finite and non-negative");
  internal::RequireNonNegative(style.margin.bottom,
                               "HuxerUI Interaction: ScrollToTop margin bottom must be finite and non-negative");
  internal::RequireNonNegative(style.margin.left,
                               "HuxerUI Interaction: ScrollToTop margin left must be finite and non-negative");

  if(const auto* tween = std::get_if<TweenSpec>(&style.scroll_motion)) {
    internal::RequireNonNegative(
        tween->duration, "HuxerUI Interaction: ScrollToTop scroll motion tween duration must be finite and non-negative");
  } else if(const auto* spring = std::get_if<SpringSpec>(&style.scroll_motion)) {
    internal::RequirePositive(
        spring->stiffness,
        "HuxerUI Interaction: ScrollToTop scroll motion spring stiffness must be finite and positive");
    internal::RequirePositive(
        spring->damping_ratio,
        "HuxerUI Interaction: ScrollToTop scroll motion spring damping ratio must be finite and positive");
  }

  internal::RequireUnitInterval(style.reveal_motion.initial_opacity,
                                "HuxerUI Interaction: ScrollToTop reveal initial opacity must be in [0, 1]");
  internal::RequireFinite(style.reveal_motion.initial_offset,
                          "HuxerUI Interaction: ScrollToTop reveal initial offset must be finite");
  internal::RequirePositive(style.reveal_motion.initial_scale,
                            "HuxerUI Interaction: ScrollToTop reveal initial scale must be finite and positive");
  internal::RequireFinite(style.reveal_motion.origin.x,
                          "HuxerUI Interaction: ScrollToTop reveal transform origin must be finite");
  internal::RequireFinite(style.reveal_motion.origin.y,
                          "HuxerUI Interaction: ScrollToTop reveal transform origin must be finite");
  internal::RequireNonNegative(style.reveal_motion.delay,
                               "HuxerUI Interaction: ScrollToTop reveal delay must be finite and non-negative");
  if(const auto* tween = std::get_if<TweenSpec>(&style.reveal_motion.motion)) {
    internal::RequireNonNegative(
        tween->duration,
        "HuxerUI Interaction: ScrollToTop reveal motion tween duration must be finite and non-negative");
  } else if(const auto* spring = std::get_if<SpringSpec>(&style.reveal_motion.motion)) {
    internal::RequirePositive(
        spring->stiffness,
        "HuxerUI Interaction: ScrollToTop reveal motion spring stiffness must be finite and positive");
    internal::RequireNonNegative(
        spring->damping_ratio,
        "HuxerUI Interaction: ScrollToTop reveal motion spring damping ratio must be finite and non-negative");
  }
}

struct ScrollToTopEnvironment {
  std::shared_ptr<detail::ScrollToTopControllerState> state;

  static ScrollToTopEnvironment Default() { return {}; }

  bool operator==(const ScrollToTopEnvironment&) const = default;
};

struct ScrollToTopEvaluation {
  bool connected = false;
  bool visible = false;
  bool scrolling_to_top = false;
  float offset = 0.0F;
  float maximum_offset = 0.0F;
  float progress = 0.0F;
};

ScrollToTopEvaluation EvaluateScrollToTop(detail::ScrollToTopControllerState& state) {
  if(!state.style.scroll_controller.has_value()) {
    state.has_last_offset = false;
    state.up_scroll_visible = false;
    return {.scrolling_to_top = state.animating};
  }
  const ScrollMetrics metrics = state.style.scroll_controller->Metrics();
  if(!state.style.scroll_controller->IsConnected() || metrics.axis != state.style.axis) {
    state.has_last_offset = false;
    state.up_scroll_visible = false;
    return {.scrolling_to_top = state.animating};
  }

  const float offset = std::max(0.0F, metrics.offset);
  const float max_offset = std::max(0.0F, metrics.maximum_offset);
  const float progress = max_offset > 0.0F ? std::clamp(offset / max_offset, 0.0F, 1.0F) : 0.0F;

  bool visible = false;
  if(!state.style.show_on_scroll_up) {
    visible = (offset >= state.style.threshold - kOffsetEpsilon)
              && (state.style.threshold > 0.0F || max_offset > state.style.target_offset + kOffsetEpsilon);
    state.has_last_offset = true;
    state.last_offset = offset;
    state.peak_offset = offset;
    state.trough_offset = offset;
    state.up_scroll_visible = false;
  } else {
    const float threshold = state.style.threshold;
    const float delta = state.style.scroll_up_delta;
    const float hide_delta = std::max(kMoveEpsilon, delta);
    if(offset < threshold - kOffsetEpsilon
       || (threshold <= 0.0F && offset <= state.style.target_offset + kOffsetEpsilon)) {
      state.up_scroll_visible = false;
      state.has_last_offset = true;
      state.last_offset = offset;
      state.peak_offset = offset;
      state.trough_offset = offset;
    } else if(!state.has_last_offset) {
      state.has_last_offset = true;
      state.last_offset = offset;
      state.peak_offset = offset;
      state.trough_offset = offset;
      state.up_scroll_visible = false;
    } else if(offset < state.last_offset - kMoveEpsilon) {
      state.last_offset = offset;
      state.trough_offset = offset;
      if(state.peak_offset - offset >= delta - kOffsetEpsilon) {
        state.up_scroll_visible = true;
      }
    } else if(offset > state.last_offset + kMoveEpsilon) {
      state.last_offset = offset;
      state.peak_offset = std::max(state.peak_offset, offset);
      if(state.up_scroll_visible && (offset - state.trough_offset >= hide_delta - kOffsetEpsilon)) {
        state.up_scroll_visible = false;
        state.peak_offset = offset;
      }
    }
    visible = state.up_scroll_visible;
  }

  return {
      .connected = true,
      .visible = visible,
      .scrolling_to_top = state.animating,
      .offset = offset,
      .maximum_offset = max_offset,
      .progress = progress,
  };
}

bool ControllerOutOfSync(const detail::ScrollToTopControllerState& state, const ScrollToTopEvaluation& eval) {
  if(state.visible.IsValid() && state.visible.Get() != eval.visible) return true;
  if(state.scrolling_to_top.IsValid() && state.scrolling_to_top.Get() != eval.scrolling_to_top) return true;
  if(state.offset.IsValid() && std::abs(state.offset.Get() - eval.offset) > kOffsetEpsilon) return true;
  if(state.progress.IsValid() && std::abs(state.progress.Get() - eval.progress) > kProgressEpsilon) return true;
  return false;
}

void ScheduleSync(detail::ScrollToTopControllerState& state) {
  if(state.sync_scheduled || !state.alive || !*state.alive || !state.post_sync) return;
  state.sync_scheduled = true;
  state.post_sync();
}

struct ScrollToTopStageData {
  HorizontalAlignment horizontal_alignment = HorizontalAlignment::End;
  VerticalAlignment vertical_alignment = VerticalAlignment::End;
  EdgeInsets margin = EdgeInsets::All(16.0F);
  bool visible = false;

  bool operator==(const ScrollToTopStageData&) const = default;
};

struct ScrollToTopStageLayoutValue {
  using Value = ScrollToTopStageData;
};

class ScrollToTopStageLayout final : public Layout<ScrollToTopStageLayout> {
public:
  using Layout::Layout;

  static LayoutResult Measure(LayoutContext& context, ViewNode& node, const Constraints& constraints) {
    LayoutResult result;
    if(node.ChildCount() == 0U) {
      return result.SetSize(constraints.Constrain(Size{}));
    }
    const Size content_size = context.Measure(node.ChildAt(0U), constraints);
    const Size stage_size = constraints.Constrain(content_size);
    result.Place(node.ChildAt(0U), Point{0.0F, 0.0F});

    const ScrollToTopStageData data = node.LayoutValueOr<ScrollToTopStageLayoutValue>(ScrollToTopStageData{});
    if(node.ChildCount() >= 2U && data.visible) {
      const float inner_width = std::max(0.0F, stage_size.width - data.margin.Horizontal());
      const float inner_height = std::max(0.0F, stage_size.height - data.margin.Vertical());
      const Constraints trigger_constraints{
          data.horizontal_alignment == HorizontalAlignment::Stretch ? inner_width : 0.0F,
          inner_width,
          data.vertical_alignment == VerticalAlignment::Stretch ? inner_height : 0.0F,
          inner_height,
      };
      const Size trigger_size = context.Measure(node.ChildAt(1U), trigger_constraints);

      float x = data.margin.left;
      switch(data.horizontal_alignment) {
        case HorizontalAlignment::Start:
        case HorizontalAlignment::Stretch:
          x = data.margin.left;
          break;
        case HorizontalAlignment::Center:
          x = data.margin.left + std::max(0.0F, inner_width - trigger_size.width) * 0.5F;
          break;
        case HorizontalAlignment::End:
          x = data.margin.left + std::max(0.0F, inner_width - trigger_size.width);
          break;
      }

      float y = data.margin.top;
      switch(data.vertical_alignment) {
        case VerticalAlignment::Start:
        case VerticalAlignment::Stretch:
          y = data.margin.top;
          break;
        case VerticalAlignment::Center:
          y = data.margin.top + std::max(0.0F, inner_height - trigger_size.height) * 0.5F;
          break;
        case VerticalAlignment::End:
          y = data.margin.top + std::max(0.0F, inner_height - trigger_size.height);
          break;
      }

      result.Place(node.ChildAt(1U), Point{x, y});
    }
    return result.SetSize(stage_size);
  }
};

struct ScrollToTopDriver {
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  std::shared_ptr<detail::ScrollToTopControllerState> state;

  bool operator==(const ScrollToTopDriver&) const = default;
};

class ScrollToTopDriver::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const ScrollToTopDriver& value) { Update(node, value); }

  ~Extension() override {
    if(state_) {
      state_->start_smooth_scroll = {};
      state_->cancel_smooth_scroll = {};
      state_->animating = false;
    }
  }

  void Update(ViewNode&, const ScrollToTopDriver& value) {
    const bool state_changed = state_ != value.state;
    if(state_ && state_changed) {
      state_->start_smooth_scroll = {};
      state_->cancel_smooth_scroll = {};
      state_->animating = false;
      animator_.Cancel();
      has_driven_offset_ = false;
    }
    state_ = value.state;
    if(state_) {
      state_->start_smooth_scroll = [this](float from, float to, const AnimationSpec& motion) {
        animator_.Start(from, to, motion);
        last_driven_offset_ = from;
        has_driven_offset_ = true;
        if(state_) {
          state_->animating = true;
          state_->needs_sync = true;
          ScheduleSync(*state_);
        }
        InvalidatePaint(PaintInvalidation::Foreground);
      };
      state_->cancel_smooth_scroll = [this]() {
        if(animator_.IsActive() || has_driven_offset_ || (state_ && state_->animating)) {
          animator_.Cancel();
          has_driven_offset_ = false;
          if(state_) {
            state_->animating = false;
            state_->needs_sync = true;
            ScheduleSync(*state_);
          }
        }
      };
      if(state_changed) {
        state_->needs_sync = true;
      }
    }
  }

  FrameResult OnFrame(ViewNode&, const FrameInfo& frame) override {
    if(!animator_.IsActive()) {
      if(has_driven_offset_ || (state_ && state_->animating)) {
        has_driven_offset_ = false;
        if(state_) {
          state_->animating = false;
          state_->needs_sync = true;
          ScheduleSync(*state_);
        }
      }
      return {};
    }
    if(!state_ || !state_->style.scroll_controller.has_value() || !state_->style.scroll_controller->IsConnected()) {
      animator_.Cancel();
      has_driven_offset_ = false;
      if(state_) {
        state_->animating = false;
        state_->needs_sync = true;
        ScheduleSync(*state_);
      }
      return {};
    }

    const ScrollController sc = *state_->style.scroll_controller;
    const ScrollMetrics before = sc.Metrics();
    if(before.axis != state_->style.axis) {
      animator_.Cancel();
      has_driven_offset_ = false;
      state_->animating = false;
      state_->needs_sync = true;
      ScheduleSync(*state_);
      return {};
    }
    if(state_->style.interrupt_on_user_scroll && has_driven_offset_
       && std::abs(before.offset - last_driven_offset_) > kInterruptEpsilon) {
      animator_.Cancel();
      has_driven_offset_ = false;
      state_->animating = false;
      state_->needs_sync = true;
      ScheduleSync(*state_);
      return {};
    }

    const FrameResult result = animator_.Advance(sc, frame, state_->style.interrupt_on_user_scroll);
    const ScrollMetrics after = sc.Metrics();
    last_driven_offset_ = after.offset;
    has_driven_offset_ = animator_.IsActive();
    if(!animator_.IsActive()) {
      state_->animating = false;
      state_->needs_sync = true;
      ScheduleSync(*state_);
    } else if(ControllerOutOfSync(*state_, EvaluateScrollToTop(*state_))) {
      ScheduleSync(*state_);
    }
    return result;
  }

  PaintInvalidation PrepareGeometry(ViewNode&, TextMeasurer&) override {
    if(!state_) return PaintInvalidation::None;
    if(animator_.IsActive() && state_->style.interrupt_on_user_scroll && has_driven_offset_
       && state_->style.scroll_controller.has_value() && state_->style.scroll_controller->IsConnected()) {
      const ScrollMetrics current = state_->style.scroll_controller->Metrics();
      if(std::abs(current.offset - last_driven_offset_) > kInterruptEpsilon) {
        animator_.Cancel();
        has_driven_offset_ = false;
        state_->animating = false;
        state_->needs_sync = true;
      }
    }
    const ScrollToTopEvaluation eval = EvaluateScrollToTop(*state_);
    if(state_->needs_sync || ControllerOutOfSync(*state_, eval)) {
      state_->needs_sync = false;
      ScheduleSync(*state_);
    }
    return PaintInvalidation::None;
  }

private:
  std::shared_ptr<detail::ScrollToTopControllerState> state_;
  internal::ScrollAnimator animator_;
  float last_driven_offset_ = 0.0F;
  bool has_driven_offset_ = false;
};

const huxerui::detail::ModifierDescriptor& ScrollToTopDriver::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<ScrollToTopDriver, ScrollToTopDriver::Extension>();
}

} // namespace

bool ScrollToTopController::IsVisible() const {
  if(!state_ || !state_->visible.IsValid()) return false;
  return state_->visible.Get();
}

bool ScrollToTopController::IsScrollingToTop() const {
  if(!state_ || !state_->scrolling_to_top.IsValid()) return false;
  return state_->scrolling_to_top.Get();
}

float ScrollToTopController::Offset() const {
  if(!state_ || !state_->offset.IsValid()) return 0.0F;
  return state_->offset.Get();
}

float ScrollToTopController::Progress() const {
  if(!state_ || !state_->progress.IsValid()) return 0.0F;
  return state_->progress.Get();
}

bool ScrollToTopController::ScrollToTop(std::optional<ScrollJumpBehavior> behavior) const {
  if(!state_ || !state_->style.scroll_controller.has_value() || !state_->style.scroll_controller->IsConnected()) {
    return false;
  }
  const ScrollController sc = *state_->style.scroll_controller;
  const ScrollMetrics metrics = sc.Metrics();
  if(metrics.axis != state_->style.axis) {
    return false;
  }
  const float current_offset = std::max(0.0F, metrics.offset);
  const float clamped_target = std::clamp(state_->style.target_offset, 0.0F, std::max(0.0F, metrics.maximum_offset));
  const ScrollJumpBehavior resolved = behavior.value_or(state_->style.jump_behavior);
  const bool zero_tween = std::holds_alternative<TweenSpec>(state_->style.scroll_motion)
                          && std::get<TweenSpec>(state_->style.scroll_motion).duration <= 0.0;

  bool accepted = false;
  if(resolved == ScrollJumpBehavior::Instant || zero_tween || !state_->start_smooth_scroll
     || std::abs(clamped_target - current_offset) <= kMoveEpsilon) {
    if(state_->cancel_smooth_scroll) {
      state_->cancel_smooth_scroll();
    }
    accepted = sc.ScrollTo(clamped_target);
    if(accepted) {
      state_->needs_sync = true;
      ScheduleSync(*state_);
    }
  } else {
    state_->start_smooth_scroll(current_offset, clamped_target, state_->style.scroll_motion);
    accepted = true;
  }

  if(accepted && state_->on_triggered) {
    auto cb = state_->on_triggered;
    cb();
  }
  return accepted;
}

void ScrollToTopController::Cancel() const {
  if(state_ && state_->cancel_smooth_scroll) {
    state_->cancel_smooth_scroll();
  }
}

ScrollToTopController UseScrollToTopController() {
  auto visible = UseState(false);
  auto scrolling_to_top = UseState(false);
  auto offset = UseState(0.0F);
  auto progress = UseState(0.0F);
  auto holder = UseState(std::make_shared<detail::ScrollToTopControllerState>());
  auto shared = holder.Get();
  shared->visible = visible;
  shared->scrolling_to_top = scrolling_to_top;
  shared->offset = offset;
  shared->progress = progress;
  return ScrollToTopController(std::move(shared));
}

View ScrollToTop(View scrollable_content, std::function<View(ScrollToTopContext)> trigger_builder,
                 ScrollToTopStyle style, std::function<void()> on_triggered) {
  if(!trigger_builder) {
    throw std::invalid_argument("HuxerUI Interaction: ScrollToTop requires a trigger builder");
  }
  ValidateStyle(style);

  return Scope([scrollable_content = std::move(scrollable_content), trigger_builder = std::move(trigger_builder),
                style = std::move(style), on_triggered = std::move(on_triggered)]() -> View {
    TaskScope tasks = UseTaskScope();
    auto alive = UseState(std::make_shared<bool>(true)).Get();
    auto local_visible = UseState(false);
    auto local_scrolling = UseState(false);
    auto local_offset = UseState(0.0F);
    auto local_progress = UseState(0.0F);
    auto local_holder = UseState(std::make_shared<detail::ScrollToTopControllerState>());

    std::shared_ptr<detail::ScrollToTopControllerState> state =
        style.controller.state_ ? style.controller.state_ : local_holder.Get();
    if(!style.controller.state_) {
      state->visible = local_visible;
      state->scrolling_to_top = local_scrolling;
      state->offset = local_offset;
      state->progress = local_progress;
    }

    const bool threshold_changed = state->style.threshold != style.threshold
                                   || state->style.show_on_scroll_up != style.show_on_scroll_up
                                   || state->style.axis != style.axis;
    state->style = style;
    state->style.controller = {};
    state->on_triggered = on_triggered;
    state->alive = alive;
    *alive = true;
    if(threshold_changed && !style.show_on_scroll_up) {
      state->up_scroll_visible = false;
    }

    std::weak_ptr<detail::ScrollToTopControllerState> weak_state = state;
    state->post_sync = [tasks, weak_state, alive]() {
      tasks.Post([weak_state, alive]() {
        if(!*alive) return;
        auto locked = weak_state.lock();
        if(!locked) return;
        locked->sync_scheduled = false;

        const ScrollToTopEvaluation eval = EvaluateScrollToTop(*locked);
        if(locked->visible.IsValid() && locked->visible.Get() != eval.visible) {
          locked->visible = eval.visible;
        }
        if(locked->scrolling_to_top.IsValid() && locked->scrolling_to_top.Get() != eval.scrolling_to_top) {
          locked->scrolling_to_top = eval.scrolling_to_top;
        }
        if(locked->offset.IsValid() && std::abs(locked->offset.Get() - eval.offset) > kOffsetEpsilon) {
          locked->offset = eval.offset;
        }
        if(locked->progress.IsValid() && std::abs(locked->progress.Get() - eval.progress) > kProgressEpsilon) {
          locked->progress = eval.progress;
        }
      });
    };

    Lifecycle(
        [weak_state, alive]() {
          *alive = true;
          return [weak_state, alive]() {
            *alive = false;
            if(auto locked = weak_state.lock()) {
              locked->post_sync = {};
              locked->sync_scheduled = false;
            }
          };
        },
        state);

    View tracked = ProvideEnvironment(ScrollToTopEnvironment{state}, scrollable_content);
    ScrollToTopController effective_controller(state);

    return Scope([tracked = std::move(tracked), trigger_builder, state, effective_controller]() -> View {
      if(state->visible.IsValid()) (void)state->visible.Get();
      if(state->scrolling_to_top.IsValid()) (void)state->scrolling_to_top.Get();
      const ScrollToTopEvaluation eval = EvaluateScrollToTop(*state);

      std::vector<View> stage_children;
      stage_children.reserve(eval.visible ? 2U : 1U);
      stage_children.push_back(tracked);
      if(eval.visible) {
        View trigger_view = trigger_builder(ScrollToTopContext{
            .visible = eval.visible,
            .scrolling_to_top = eval.scrolling_to_top,
            .offset = eval.offset,
            .progress = eval.progress,
            .controller = effective_controller,
        });
        stage_children.push_back(Appear(std::move(trigger_view), state->style.reveal_motion));
      }

      const ScrollToTopStageData stage_data{
          .horizontal_alignment = state->style.horizontal_alignment,
          .vertical_alignment = state->style.vertical_alignment,
          .margin = state->style.margin,
          .visible = eval.visible,
      };
      return ScrollToTopStageLayout(std::move(stage_children))
          .LayoutValue<ScrollToTopStageLayoutValue>(stage_data)
          .With(ClipChildren{}, ScrollToTopDriver{.state = state});
    });
  });
}

View ScrollToTop(View scrollable_content, View trigger, ScrollToTopStyle style, std::function<void()> on_triggered) {
  ValidateStyle(style);
  return ScrollToTop(
      std::move(scrollable_content),
      [trigger = std::move(trigger)](ScrollToTopContext ctx) -> View {
        View interactive = trigger;
        return std::move(interactive)
            .OnClick([ctx]() { ctx.ScrollToTop(); })
            .With(PointerCursor{PointerCursorKind::Hand});
      },
      std::move(style), std::move(on_triggered));
}

View ScrollToTopTrigger(View trigger, ScrollToTopController controller,
                        std::optional<ScrollJumpBehavior> behavior, std::function<void()> on_triggered) {
  return Scope([trigger = std::move(trigger), controller = std::move(controller), behavior,
                on_triggered = std::move(on_triggered)]() -> View {
    const auto& env = UseEnvironment<ScrollToTopEnvironment>();
    const std::shared_ptr<detail::ScrollToTopControllerState> bound = controller.state_ ? controller.state_ : env.state;
    View interactive = trigger;
    return std::move(interactive)
        .OnClick([bound, behavior, on_triggered]() {
          if(bound && ScrollToTopController(bound).ScrollToTop(behavior)) {
            if(on_triggered) {
              auto cb = on_triggered;
              cb();
            }
          }
        })
        .With(PointerCursor{PointerCursorKind::Hand});
  });
}

} // namespace huxerui::interaction
