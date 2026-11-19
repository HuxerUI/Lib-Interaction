#include <huxerui/interaction/gesture/swipedeck.h>

#include "../internal/motion_group.h"
#include "../internal/validate.h"

#include <huxerui/gesture.h>
#include <huxerui/layout.h>
#include <huxerui/lifecycle.h>
#include <huxerui/modifier.h>
#include <huxerui/state.h>
#include <huxerui/task.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <variant>

namespace huxerui::interaction {

namespace detail {

struct SwipeDeckControllerState {
  enum class Phase { Idle, Dragging, Returning, Exiting, Rewinding };

  struct HistoryEntry {
    std::size_t index = 0;
    SwipeDirection direction = SwipeDirection::Right;
  };

  State<std::size_t> top_index;
  State<std::size_t> total_count;
  State<bool> dragging;
  State<Point> offset;
  State<bool> can_rewind;
  SwipeDeckStyle style;
  std::size_t count = 0;
  bool sync_scheduled = false;
  std::shared_ptr<bool> alive;
  std::function<void()> post_sync;
  std::function<void(std::size_t, SwipeDirection)> on_swiped;
  std::vector<HistoryEntry> history;
  Phase phase = Phase::Idle;
  SwipeDirection pending_exit_direction = SwipeDirection::None;
  std::size_t pending_exit_index = 0;
  std::function<void(Point from, Point vel, Point target, const AnimationSpec& motion, std::size_t idx,
                     SwipeDirection dir)>
      start_exit;
  std::function<void(Point from, Point vel, const AnimationSpec& motion)> start_return;
  std::function<void(Point from, const AnimationSpec& motion)> start_rewind;
  std::function<void()> cancel_motion;
};

} // namespace detail

namespace {

void ValidateMotion(const AnimationSpec& spec, const char* context_msg) {
  if(const auto* tween = std::get_if<TweenSpec>(&spec)) {
    internal::RequireNonNegative(tween->duration, context_msg);
  } else if(const auto* spring = std::get_if<SpringSpec>(&spec)) {
    internal::RequirePositive(spring->stiffness, context_msg);
    internal::RequirePositive(spring->damping_ratio, context_msg);
  }
}

void ValidateStyle(const SwipeDeckStyle& style) {
  if(style.visible_cards == 0) {
    throw std::invalid_argument("HuxerUI Interaction: SwipeDeck visible_cards must be at least 1");
  }
  if(style.allowed_directions == SwipeDirectionMask::None) {
    throw std::invalid_argument("HuxerUI Interaction: SwipeDeck allowed_directions must not be None");
  }
  internal::RequirePositive(style.swipe_threshold,
                            "HuxerUI Interaction: SwipeDeck swipe threshold must be finite and positive");
  internal::RequireNonNegative(style.minimum_velocity,
                               "HuxerUI Interaction: SwipeDeck minimum velocity must be finite and non-negative");
  internal::RequirePositive(style.exit_distance,
                            "HuxerUI Interaction: SwipeDeck exit distance must be finite and positive");
  internal::RequireNonNegative(style.maximum_rotation,
                               "HuxerUI Interaction: SwipeDeck maximum rotation must be finite and non-negative");
  if(!std::isfinite(style.depth_scale_step) || style.depth_scale_step < 0.0F || style.depth_scale_step >= 0.5F) {
    throw std::invalid_argument("HuxerUI Interaction: SwipeDeck depth scale step must be in [0, 0.5)");
  }
  internal::RequireFinite(style.depth_offset_y,
                          "HuxerUI Interaction: SwipeDeck depth offset y must be finite");
  internal::RequireUnitInterval(style.depth_opacity_step,
                                "HuxerUI Interaction: SwipeDeck depth opacity step must be in [0, 1]");
  internal::RequireNonNegative(style.maximum_velocity,
                               "HuxerUI Interaction: SwipeDeck maximum velocity must be finite and non-negative");
  ValidateMotion(style.return_motion,
                 "HuxerUI Interaction: SwipeDeck return motion parameters must be valid and positive");
  ValidateMotion(style.exit_motion,
                 "HuxerUI Interaction: SwipeDeck exit motion parameters must be valid and positive");
  ValidateMotion(style.rewind_motion,
                 "HuxerUI Interaction: SwipeDeck rewind motion parameters must be valid and positive");
}

std::optional<Axis> GestureAxisFor(SwipeDirectionMask mask) noexcept {
  const bool allow_h = HasDirection(mask, SwipeDirectionMask::Left) || HasDirection(mask, SwipeDirectionMask::Right);
  const bool allow_v = HasDirection(mask, SwipeDirectionMask::Up) || HasDirection(mask, SwipeDirectionMask::Down);
  if(allow_h && !allow_v) return Axis::Horizontal;
  if(allow_v && !allow_h) return Axis::Vertical;
  return std::nullopt;
}

struct LiveSwipeReading {
  SwipeDirection direction = SwipeDirection::None;
  float progress = 0.0F;
};

LiveSwipeReading ResolveLiveDirection(Point offset, SwipeDirectionMask allowed, float threshold) noexcept {
  if(threshold <= 0.0F) return {};
  const float ax = std::abs(offset.x);
  const float ay = std::abs(offset.y);
  SwipeDirection dir = SwipeDirection::None;
  float mag = 0.0F;
  const bool prefer_h = ax >= ay;
  auto pick_h = [&]() {
    if(offset.x < -1.0F && HasDirection(allowed, SwipeDirectionMask::Left)) {
      dir = SwipeDirection::Left;
      mag = -offset.x;
    } else if(offset.x > 1.0F && HasDirection(allowed, SwipeDirectionMask::Right)) {
      dir = SwipeDirection::Right;
      mag = offset.x;
    }
  };
  auto pick_v = [&]() {
    if(offset.y < -1.0F && HasDirection(allowed, SwipeDirectionMask::Up)) {
      dir = SwipeDirection::Up;
      mag = -offset.y;
    } else if(offset.y > 1.0F && HasDirection(allowed, SwipeDirectionMask::Down)) {
      dir = SwipeDirection::Down;
      mag = offset.y;
    }
  };
  if(prefer_h) {
    pick_h();
    if(dir == SwipeDirection::None) pick_v();
  } else {
    pick_v();
    if(dir == SwipeDirection::None) pick_h();
  }
  const float radial = std::clamp(std::hypot(offset.x, offset.y) / threshold, 0.0F, 1.0F);
  return {dir, dir != SwipeDirection::None ? std::clamp(mag / threshold, 0.0F, 1.0F) : radial};
}

SwipeDirection ResolveCommitDirection(Point offset, Point velocity, const SwipeDeckStyle& style) noexcept {
  SwipeDirection best_dir = SwipeDirection::None;
  float best_score = 0.0F;
  const float vel_denom = std::max(1.0F, style.minimum_velocity);

  auto consider = [&](SwipeDirectionMask flag, SwipeDirection dir, float disp, float vel) {
    if(!HasDirection(style.allowed_directions, flag)) return;
    const bool by_dist = disp >= style.swipe_threshold;
    const bool by_vel = style.minimum_velocity > 0.0F && vel >= style.minimum_velocity && disp >= -12.0F;
    if(!by_dist && !by_vel) return;
    const float score = std::max(disp / style.swipe_threshold, vel / vel_denom);
    if(score > best_score) {
      best_score = score;
      best_dir = dir;
    }
  };

  consider(SwipeDirectionMask::Left, SwipeDirection::Left, -offset.x, -velocity.x);
  consider(SwipeDirectionMask::Right, SwipeDirection::Right, offset.x, velocity.x);
  consider(SwipeDirectionMask::Up, SwipeDirection::Up, -offset.y, -velocity.y);
  consider(SwipeDirectionMask::Down, SwipeDirection::Down, offset.y, velocity.y);
  return best_dir;
}

Point ExitTargetFor(SwipeDirection dir, Point current_offset, float exit_distance) noexcept {
  switch(dir) {
  case SwipeDirection::Left:
    return {-exit_distance, current_offset.y * 0.35F};
  case SwipeDirection::Right:
    return {exit_distance, current_offset.y * 0.35F};
  case SwipeDirection::Up:
    return {current_offset.x * 0.35F, -exit_distance};
  case SwipeDirection::Down:
    return {current_offset.x * 0.35F, exit_distance};
  case SwipeDirection::None:
    return {};
  }
  return {};
}

Point ClampVelocity(Point velocity, float max_vel) noexcept {
  const float vx = std::isfinite(velocity.x) ? velocity.x : 0.0F;
  const float vy = std::isfinite(velocity.y) ? velocity.y : 0.0F;
  if(max_vel <= 0.0F) return {0.0F, 0.0F};
  const float mag = std::hypot(vx, vy);
  if(mag <= max_vel) return {vx, vy};
  const float scale = max_vel / mag;
  return {vx * scale, vy * scale};
}

struct SwipeDeckDriver {
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  std::shared_ptr<detail::SwipeDeckControllerState> state;

  bool operator==(const SwipeDeckDriver&) const = default;
};

class SwipeDeckDriver::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const SwipeDeckDriver& value) { Update(node, value); }

  ~Extension() override { ClearCallbacks(state_); }

  void Update(ViewNode&, const SwipeDeckDriver& value) {
    if(state_ && state_ != value.state) {
      ClearCallbacks(state_);
      animating_ = false;
    }
    state_ = value.state;
    if(!state_) return;

    state_->start_exit = [this](Point from, Point vel, Point target, const AnimationSpec& motion, std::size_t idx,
                                SwipeDirection dir) {
      if(!state_) return;
      state_->phase = detail::SwipeDeckControllerState::Phase::Exiting;
      state_->pending_exit_index = idx;
      state_->pending_exit_direction = dir;
      motion_x_.Seek(from.x, vel.x);
      motion_y_.Seek(from.y, vel.y);
      motion_x_.AnimateTo(target.x, motion);
      motion_y_.AnimateTo(target.y, motion);
      target_ = target;
      animating_ = true;
      InvalidatePaint(PaintInvalidation::Foreground);
    };
    state_->start_return = [this](Point from, Point vel, const AnimationSpec& motion) {
      if(!state_) return;
      state_->phase = detail::SwipeDeckControllerState::Phase::Returning;
      motion_x_.Seek(from.x, vel.x);
      motion_y_.Seek(from.y, vel.y);
      motion_x_.AnimateTo(0.0F, motion);
      motion_y_.AnimateTo(0.0F, motion);
      target_ = Point{};
      animating_ = true;
      InvalidatePaint(PaintInvalidation::Foreground);
    };
    state_->start_rewind = [this](Point from, const AnimationSpec& motion) {
      if(!state_) return;
      state_->phase = detail::SwipeDeckControllerState::Phase::Rewinding;
      motion_x_.Set(from.x);
      motion_y_.Set(from.y);
      motion_x_.AnimateTo(0.0F, motion);
      motion_y_.AnimateTo(0.0F, motion);
      target_ = Point{};
      animating_ = true;
      InvalidatePaint(PaintInvalidation::Foreground);
    };
    state_->cancel_motion = [this]() { animating_ = false; };
  }

  PaintInvalidation PrepareGeometry(ViewNode&, TextMeasurer&) override {
    if(state_ && state_->total_count.IsValid() && state_->total_count.Get() != state_->count
       && !state_->sync_scheduled && state_->alive && *state_->alive && state_->post_sync) {
      state_->sync_scheduled = true;
      state_->post_sync();
    }
    return PaintInvalidation::None;
  }

  FrameResult OnFrame(ViewNode&, const FrameInfo& frame) override {
    if(!animating_ || !state_) return {};
    const MotionAdvanceResult rx = motion_x_.Advance(frame);
    const MotionAdvanceResult ry = motion_y_.Advance(frame);
    internal::FrameAccumulator acc;
    acc.Add(rx);
    acc.Add(ry);

    const bool running = motion_x_.IsRunning() || motion_y_.IsRunning();
    const Point next = running ? Point{motion_x_.Value(), motion_y_.Value()} : target_;
    if(acc.changed && state_->offset.IsValid() && state_->offset.Get() != next) {
      state_->offset = next;
    }

    if(!running) {
      animating_ = false;
      const auto completed_phase = state_->phase;
      state_->phase = detail::SwipeDeckControllerState::Phase::Idle;
      if(state_->offset.IsValid() && state_->offset.Get() != Point{}) {
        state_->offset = Point{};
      }
      if(completed_phase == detail::SwipeDeckControllerState::Phase::Exiting) {
        const std::size_t idx = state_->pending_exit_index;
        const SwipeDirection dir = state_->pending_exit_direction;
        state_->pending_exit_direction = SwipeDirection::None;
        state_->history.push_back({idx, dir});
        if(state_->top_index.IsValid()) state_->top_index = idx + 1;
        if(state_->can_rewind.IsValid()) state_->can_rewind = true;
        if(auto cb = state_->on_swiped) {
          cb(idx, dir);
        }
      }
      return {};
    }
    return acc.Result();
  }

private:
  static void ClearCallbacks(const std::shared_ptr<detail::SwipeDeckControllerState>& state) {
    if(!state) return;
    state->start_exit = {};
    state->start_return = {};
    state->start_rewind = {};
    state->cancel_motion = {};
  }

  std::shared_ptr<detail::SwipeDeckControllerState> state_;
  MotionController motion_x_{0.0F};
  MotionController motion_y_{0.0F};
  Point target_{};
  bool animating_ = false;
};

const huxerui::detail::ModifierDescriptor& SwipeDeckDriver::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<SwipeDeckDriver, SwipeDeckDriver::Extension>();
}

} // namespace

std::size_t SwipeDeckController::TopIndex() const {
  if(!state_ || !state_->top_index.IsValid()) return 0;
  return state_->top_index.Get();
}

std::size_t SwipeDeckController::RemainingCount() const {
  if(!state_ || !state_->top_index.IsValid()) return 0;
  if(state_->total_count.IsValid()) (void)state_->total_count.Get();
  const std::size_t top = state_->top_index.Get();
  return state_->count > top ? state_->count - top : 0U;
}

bool SwipeDeckController::IsDragging() const {
  if(!state_ || !state_->dragging.IsValid()) return false;
  return state_->dragging.Get();
}

bool SwipeDeckController::CanRewind() const {
  if(!state_ || !state_->can_rewind.IsValid()) return false;
  return state_->can_rewind.Get();
}

float SwipeDeckController::DragProgress() const {
  if(!state_ || !state_->offset.IsValid()) return 0.0F;
  return ResolveLiveDirection(state_->offset.Get(), state_->style.allowed_directions, state_->style.swipe_threshold)
      .progress;
}

SwipeDirection SwipeDeckController::DragDirection() const {
  if(!state_ || !state_->offset.IsValid()) return SwipeDirection::None;
  return ResolveLiveDirection(state_->offset.Get(), state_->style.allowed_directions, state_->style.swipe_threshold)
      .direction;
}

bool SwipeDeckController::Swipe(SwipeDirection direction) const {
  if(!state_ || direction == SwipeDirection::None || !state_->top_index.IsValid()) return false;
  if(state_->phase == detail::SwipeDeckControllerState::Phase::Exiting
     || state_->phase == detail::SwipeDeckControllerState::Phase::Rewinding) {
    return false;
  }
  const std::size_t top = state_->top_index.Get();
  if(top >= state_->count) return false;
  if(state_->cancel_motion) state_->cancel_motion();
  if(state_->dragging.IsValid()) state_->dragging = false;

  const Point current = state_->offset.IsValid() ? state_->offset.Get() : Point{};
  const Point target = ExitTargetFor(direction, current, state_->style.exit_distance);
  if(state_->start_exit) {
    state_->start_exit(current, {}, target, state_->style.exit_motion, top, direction);
  } else {
    state_->history.push_back({top, direction});
    state_->top_index = top + 1;
    if(state_->offset.IsValid()) state_->offset = Point{};
    if(state_->can_rewind.IsValid()) state_->can_rewind = true;
    if(auto cb = state_->on_swiped) cb(top, direction);
  }
  return true;
}

bool SwipeDeckController::Rewind() const {
  if(!state_ || !state_->top_index.IsValid() || state_->history.empty()) return false;
  if(state_->phase == detail::SwipeDeckControllerState::Phase::Exiting
     || state_->phase == detail::SwipeDeckControllerState::Phase::Rewinding) {
    return false;
  }
  if(state_->cancel_motion) state_->cancel_motion();
  if(state_->dragging.IsValid()) state_->dragging = false;

  const auto entry = state_->history.back();
  state_->history.pop_back();
  const Point from = ExitTargetFor(entry.direction, Point{}, state_->style.exit_distance);
  state_->top_index = entry.index;
  if(state_->can_rewind.IsValid()) state_->can_rewind = !state_->history.empty();
  if(state_->offset.IsValid()) state_->offset = from;
  if(state_->start_rewind) {
    state_->start_rewind(from, state_->style.rewind_motion);
  } else if(state_->offset.IsValid()) {
    state_->offset = Point{};
  }
  return true;
}

void SwipeDeckController::Reset() const {
  if(!state_) return;
  if(state_->cancel_motion) state_->cancel_motion();
  state_->phase = detail::SwipeDeckControllerState::Phase::Idle;
  state_->pending_exit_direction = SwipeDirection::None;
  state_->history.clear();
  if(state_->dragging.IsValid()) state_->dragging = false;
  if(state_->offset.IsValid()) state_->offset = Point{};
  if(state_->top_index.IsValid()) state_->top_index = 0U;
  if(state_->can_rewind.IsValid()) state_->can_rewind = false;
}

SwipeDeckController UseSwipeDeckController(std::size_t initial_index) {
  auto top_index = UseState(initial_index);
  auto total_count = UseState<std::size_t>(0);
  auto dragging = UseState(false);
  auto offset = UseState(Point{});
  auto can_rewind = UseState(false);
  auto holder = UseState(std::make_shared<detail::SwipeDeckControllerState>());
  auto shared = holder.Get();
  shared->top_index = top_index;
  shared->total_count = total_count;
  shared->dragging = dragging;
  shared->offset = offset;
  shared->can_rewind = can_rewind;
  return SwipeDeckController(std::move(shared));
}

View SwipeDeck(std::size_t count, std::function<View(SwipeDeckCardContext)> card_builder,
               std::function<void(std::size_t index, SwipeDirection direction)> on_swiped, SwipeDeckStyle style,
               std::function<View()> empty_builder) {
  ValidateStyle(style);
  if(count > 0 && !card_builder) {
    throw std::invalid_argument("HuxerUI Interaction: SwipeDeck requires a card builder");
  }
  if(style.controller.state_) {
    style.controller.state_->count = count;
  }

  return Scope([count, card_builder = std::move(card_builder), on_swiped = std::move(on_swiped),
                style = std::move(style), empty_builder = std::move(empty_builder)]() -> View {
    TaskScope tasks = UseTaskScope();
    auto alive = UseState(std::make_shared<bool>(true)).Get();
    auto local_top = UseState<std::size_t>(0);
    auto local_total = UseState(count);
    auto local_dragging = UseState(false);
    auto local_offset = UseState(Point{});
    auto local_can_rewind = UseState(false);
    auto local_holder = UseState(std::make_shared<detail::SwipeDeckControllerState>());

    std::shared_ptr<detail::SwipeDeckControllerState> state =
        style.controller.state_ ? style.controller.state_ : local_holder.Get();
    if(!style.controller.state_) {
      state->top_index = local_top;
      state->total_count = local_total;
      state->dragging = local_dragging;
      state->offset = local_offset;
      state->can_rewind = local_can_rewind;
    }
    state->style = style;
    state->style.controller = {};
    state->count = count;
    state->on_swiped = on_swiped;
    state->alive = alive;
    *alive = true;

    std::weak_ptr<detail::SwipeDeckControllerState> weak_state = state;
    state->post_sync = [tasks, weak_state, alive]() {
      tasks.Post([weak_state, alive]() {
        if(!*alive) return;
        auto locked = weak_state.lock();
        if(!locked) return;
        locked->sync_scheduled = false;
        if(locked->total_count.IsValid() && locked->total_count.Get() != locked->count) {
          locked->total_count = locked->count;
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

    const std::size_t top = state->top_index.IsValid() ? state->top_index.Get() : 0U;
    if(top >= count) {
      View empty = empty_builder ? empty_builder() : View(Stack{});
      return Stack{std::move(empty)}.With(SwipeDeckDriver{.state = state});
    }

    const Point current_offset = state->offset.IsValid() ? state->offset.Get() : Point{};
    const bool is_dragging = state->dragging.IsValid() ? state->dragging.Get() : false;
    const LiveSwipeReading live =
        ResolveLiveDirection(current_offset, style.allowed_directions, style.swipe_threshold);
    const float promote = std::clamp(std::hypot(current_offset.x, current_offset.y) / style.swipe_threshold,
                                     0.0F, 1.0F);
    const Point norm_offset{current_offset.x / style.swipe_threshold, current_offset.y / style.swipe_threshold};

    const std::size_t max_d = std::min(style.visible_cards, count - top - 1);
    std::vector<View> layers;
    layers.reserve(max_d + 1);

    for(std::size_t rev = 0; rev <= max_d; ++rev) {
      const std::size_t depth = max_d - rev;
      const std::size_t card_idx = top + depth;
      const bool is_top = depth == 0;

      SwipeDeckCardContext ctx{
          .index = card_idx,
          .depth = depth,
          .is_top = is_top,
          .dragging = is_top && is_dragging,
          .progress = is_top ? live.progress : promote,
          .normalized_offset = is_top ? norm_offset : Point{},
          .direction = is_top ? live.direction : SwipeDirection::None,
      };
      View card = Stack{card_builder(ctx)};

      if(!is_top) {
        const float eff_depth = std::max(0.0F, static_cast<float>(depth) - promote);
        const float scale = std::max(0.1F, 1.0F - style.depth_scale_step * eff_depth);
        const float y_off = style.depth_offset_y * eff_depth;
        float alpha = std::clamp(1.0F - style.depth_opacity_step * eff_depth, 0.0F, 1.0F);
        if(depth == style.visible_cards) {
          alpha *= promote;
        }
        card = std::move(card)
                   .With(Offset(Point{0.0F, y_off}), Scale(scale), Opacity(alpha))
                   .Key(static_cast<std::uint64_t>(card_idx));
      } else {
        const float rotation =
            std::clamp(current_offset.x / style.swipe_threshold, -1.5F, 1.5F) * style.maximum_rotation;
        float alpha = 1.0F;
        if(state->phase == detail::SwipeDeckControllerState::Phase::Exiting) {
          const float travel = std::hypot(current_offset.x, current_offset.y);
          const float fade_start = style.exit_distance * 0.6F;
          if(travel > fade_start) {
            alpha = std::clamp(1.0F - (travel - fade_start) / (style.exit_distance - fade_start), 0.0F, 1.0F);
          }
        }
        card = std::move(card)
                   .With(Offset(current_offset), Rotation(rotation), Opacity(alpha),
                         PointerCursor{is_dragging ? PointerCursorKind::Grabbing : style.cursor},
                         DragGesture{.axis = GestureAxisFor(style.allowed_directions)})
                   .On<DragEvents::Started>([state](const DragEvent&) {
                     if(state->phase == detail::SwipeDeckControllerState::Phase::Exiting
                        || state->phase == detail::SwipeDeckControllerState::Phase::Rewinding) {
                       return;
                     }
                     if(state->cancel_motion) state->cancel_motion();
                     state->phase = detail::SwipeDeckControllerState::Phase::Dragging;
                     if(state->dragging.IsValid()) state->dragging = true;
                   })
                   .On<DragEvents::Changed>([state](const DragEvent& event) {
                     if(!state->dragging.IsValid() || !state->dragging.Get()) return;
                     if(state->offset.IsValid()) state->offset = event.translation;
                   })
                   .On<DragEvents::Ended>([state](const DragEvent& event) {
                     if(!state->dragging.IsValid() || !state->dragging.Get()) return;
                     state->dragging = false;
                     const Point current = event.translation;
                     const Point vel = ClampVelocity(event.velocity, state->style.maximum_velocity);
                     const SwipeDirection commit_dir = ResolveCommitDirection(current, vel, state->style);
                     if(commit_dir == SwipeDirection::None) {
                       if(state->start_return) {
                         state->start_return(current, vel, state->style.return_motion);
                       } else {
                         state->phase = detail::SwipeDeckControllerState::Phase::Idle;
                         if(state->offset.IsValid()) state->offset = Point{};
                       }
                       return;
                     }
                     const std::size_t idx = state->top_index.IsValid() ? state->top_index.Get() : 0U;
                     const Point target = ExitTargetFor(commit_dir, current, state->style.exit_distance);
                     if(state->start_exit) {
                       state->start_exit(current, vel, target, state->style.exit_motion, idx, commit_dir);
                     } else {
                       state->history.push_back({idx, commit_dir});
                       if(state->top_index.IsValid()) state->top_index = idx + 1;
                       if(state->offset.IsValid()) state->offset = Point{};
                       if(state->can_rewind.IsValid()) state->can_rewind = true;
                       if(auto cb = state->on_swiped) cb(idx, commit_dir);
                     }
                   })
                   .On<DragEvents::Canceled>([state](const DragEvent&) {
                     if(state->dragging.IsValid()) state->dragging = false;
                     if(state->phase == detail::SwipeDeckControllerState::Phase::Dragging) {
                       const Point current = state->offset.IsValid() ? state->offset.Get() : Point{};
                       if(state->start_return) {
                         state->start_return(current, {}, state->style.return_motion);
                       } else {
                         state->phase = detail::SwipeDeckControllerState::Phase::Idle;
                         if(state->offset.IsValid()) state->offset = Point{};
                       }
                     }
                   })
                   .Key(static_cast<std::uint64_t>(card_idx));
      }

      layers.push_back(std::move(card));
    }

    return Stack{std::move(layers)}.With(SwipeDeckDriver{.state = state});
  });
}

View SwipeDeck(std::vector<View> cards, std::function<void(std::size_t index, SwipeDirection direction)> on_swiped,
               SwipeDeckStyle style, View empty_view) {
  ValidateStyle(style);
  for(const View& card : cards) {
    if(!card) {
      throw std::invalid_argument("HuxerUI Interaction: SwipeDeck cards must not contain empty Views");
    }
  }
  const std::size_t count = cards.size();
  auto shared_cards = std::make_shared<std::vector<View>>(std::move(cards));
  std::function<View()> empty_builder;
  if(empty_view) {
    empty_builder = [empty_view = std::move(empty_view)]() -> View { return empty_view; };
  }
  return SwipeDeck(
      count,
      [shared_cards = std::move(shared_cards)](SwipeDeckCardContext ctx) -> View { return (*shared_cards)[ctx.index]; },
      std::move(on_swiped), std::move(style), std::move(empty_builder));
}

} // namespace huxerui::interaction
