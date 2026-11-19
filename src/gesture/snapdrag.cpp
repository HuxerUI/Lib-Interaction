#include <huxerui/interaction/gesture/snapdrag.h>

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
#include <limits>
#include <stdexcept>
#include <utility>
#include <variant>

namespace huxerui::interaction {

namespace detail {

struct SnapDragControllerState {
  State<bool> dragging;
  State<Point> position;
  State<Point> normalized;
  State<std::size_t> active_anchor;
  SnapDragStyle style;
  std::function<void(std::size_t, Point)> on_snapped;
  Size stage_size{};
  Size item_size{};
  bool has_stage = false;
  bool has_item = false;
  bool positioned_once = false;
  Point rest_normalized{};
  Point drag_start_pos{};
  Point pre_drag_normalized{};
  std::size_t pre_drag_anchor = 0;
  bool needs_sync = false;
  bool sync_scheduled = false;
  std::shared_ptr<bool> alive;
  std::function<void()> post_sync;
  std::function<void(Point from, Point velocity, Point to, const AnimationSpec& motion)> animate_to;
  std::function<void(Point to)> snap_immediate;
  std::function<void()> cancel_animation;
};

} // namespace detail

namespace {

void ValidateStyle(const SnapDragStyle& style) {
  internal::RequireNonNegative(style.margin, "HuxerUI Interaction: SnapDrag margin must be finite and non-negative");
  internal::RequireNonNegative(style.projection_time,
                               "HuxerUI Interaction: SnapDrag projection time must be finite and non-negative");
  internal::RequireNonNegative(style.maximum_velocity,
                               "HuxerUI Interaction: SnapDrag maximum velocity must be finite and non-negative");
  internal::RequirePositive(style.drag_scale, "HuxerUI Interaction: SnapDrag drag scale must be finite and positive");
  if(style.mode == SnapMode::Custom) {
    if(style.anchors.empty()) {
      throw std::invalid_argument("HuxerUI Interaction: SnapDrag Custom mode requires at least one anchor");
    }
    for(const Point& anchor : style.anchors) {
      internal::RequireUnitInterval(anchor.x,
                                    "HuxerUI Interaction: SnapDrag custom anchor x must be finite and in [0, 1]");
      internal::RequireUnitInterval(anchor.y,
                                    "HuxerUI Interaction: SnapDrag custom anchor y must be finite and in [0, 1]");
    }
  }
  if(const auto* tween = std::get_if<TweenSpec>(&style.snap_motion)) {
    internal::RequireNonNegative(tween->duration,
                                 "HuxerUI Interaction: SnapDrag tween duration must be finite and non-negative");
  } else if(const auto* spring = std::get_if<SpringSpec>(&style.snap_motion)) {
    internal::RequirePositive(spring->stiffness,
                              "HuxerUI Interaction: SnapDrag spring stiffness must be finite and positive");
    internal::RequirePositive(spring->damping_ratio,
                              "HuxerUI Interaction: SnapDrag spring damping ratio must be finite and positive");
  }
}

struct TravelBounds {
  float min_x = 0.0F;
  float max_x = 0.0F;
  float min_y = 0.0F;
  float max_y = 0.0F;

  [[nodiscard]] Point Clamp(Point p) const noexcept {
    return {std::clamp(p.x, min_x, max_x), std::clamp(p.y, min_y, max_y)};
  }
};

TravelBounds ComputeTravelBounds(Size stage, Size item, float margin) noexcept {
  const float min_x = std::min(margin, std::max(0.0F, (stage.width - item.width) * 0.5F));
  const float min_y = std::min(margin, std::max(0.0F, (stage.height - item.height) * 0.5F));
  const float max_x = std::max(min_x, stage.width - item.width - margin);
  const float max_y = std::max(min_y, stage.height - item.height - margin);
  return {min_x, max_x, min_y, max_y};
}

Point NormalizedToPixel(const TravelBounds& bounds, Point norm) noexcept {
  const float nx = std::clamp(norm.x, 0.0F, 1.0F);
  const float ny = std::clamp(norm.y, 0.0F, 1.0F);
  return {
      bounds.min_x + (bounds.max_x - bounds.min_x) * nx,
      bounds.min_y + (bounds.max_y - bounds.min_y) * ny,
  };
}

Point PixelToNormalized(const TravelBounds& bounds, Point px) noexcept {
  const float span_x = bounds.max_x - bounds.min_x;
  const float span_y = bounds.max_y - bounds.min_y;
  return {
      span_x > 1e-4F ? std::clamp((px.x - bounds.min_x) / span_x, 0.0F, 1.0F) : 0.0F,
      span_y > 1e-4F ? std::clamp((px.y - bounds.min_y) / span_y, 0.0F, 1.0F) : 0.0F,
  };
}

std::optional<Point> AnchorToNormalized(const SnapDragStyle& style, std::size_t index, Point fallback_norm) noexcept {
  switch(style.mode) {
  case SnapMode::Corners:
    switch(index) {
    case 0:
      return Point{0.0F, 0.0F};
    case 1:
      return Point{1.0F, 0.0F};
    case 2:
      return Point{0.0F, 1.0F};
    case 3:
      return Point{1.0F, 1.0F};
    default:
      return std::nullopt;
    }
  case SnapMode::HorizontalEdges:
    if(index > 1) return std::nullopt;
    return Point{index == 0 ? 0.0F : 1.0F, std::clamp(fallback_norm.y, 0.0F, 1.0F)};
  case SnapMode::VerticalEdges:
    if(index > 1) return std::nullopt;
    return Point{std::clamp(fallback_norm.x, 0.0F, 1.0F), index == 0 ? 0.0F : 1.0F};
  case SnapMode::Custom:
    if(index >= style.anchors.size()) return std::nullopt;
    return style.anchors[index];
  }
  return std::nullopt;
}

struct SnapTarget {
  std::size_t index = 0;
  Point normalized{};
  Point pixel{};
};

SnapTarget ResolveNearestSnap(const SnapDragStyle& style, const TravelBounds& bounds, Point projected_px) noexcept {
  const Point clamped_px = bounds.Clamp(projected_px);
  const Point clamped_norm = PixelToNormalized(bounds, clamped_px);
  switch(style.mode) {
  case SnapMode::Corners: {
    static constexpr Point kCorners[4] = {{0.0F, 0.0F}, {1.0F, 0.0F}, {0.0F, 1.0F}, {1.0F, 1.0F}};
    std::size_t best_idx = 0;
    float best_dist = std::numeric_limits<float>::infinity();
    Point best_px = NormalizedToPixel(bounds, kCorners[0]);
    for(std::size_t i = 0; i < 4; ++i) {
      const Point cand = NormalizedToPixel(bounds, kCorners[i]);
      const float dx = cand.x - clamped_px.x;
      const float dy = cand.y - clamped_px.y;
      const float d2 = dx * dx + dy * dy;
      if(d2 < best_dist) {
        best_dist = d2;
        best_idx = i;
        best_px = cand;
      }
    }
    return {best_idx, kCorners[best_idx], best_px};
  }
  case SnapMode::HorizontalEdges: {
    const std::size_t idx = clamped_norm.x >= 0.5F ? 1U : 0U;
    const Point norm{idx == 0 ? 0.0F : 1.0F, clamped_norm.y};
    return {idx, norm, NormalizedToPixel(bounds, norm)};
  }
  case SnapMode::VerticalEdges: {
    const std::size_t idx = clamped_norm.y >= 0.5F ? 1U : 0U;
    const Point norm{clamped_norm.x, idx == 0 ? 0.0F : 1.0F};
    return {idx, norm, NormalizedToPixel(bounds, norm)};
  }
  case SnapMode::Custom: {
    std::size_t best_idx = 0;
    float best_dist = std::numeric_limits<float>::infinity();
    Point best_px{};
    Point best_norm{};
    for(std::size_t i = 0; i < style.anchors.size(); ++i) {
      const Point cand = NormalizedToPixel(bounds, style.anchors[i]);
      const float dx = cand.x - projected_px.x;
      const float dy = cand.y - projected_px.y;
      const float d2 = dx * dx + dy * dy;
      if(d2 < best_dist) {
        best_dist = d2;
        best_idx = i;
        best_norm = style.anchors[i];
        best_px = cand;
      }
    }
    return {best_idx, best_norm, best_px};
  }
  }
  return {};
}

class SnapDragStageLayout final : public Layout<SnapDragStageLayout> {
public:
  using Layout::Layout;

  static LayoutResult Measure(LayoutContext& context, ViewNode& node, Constraints constraints) {
    if(node.ChildCount() == 0) {
      return LayoutResult{}.SetSize(constraints.Constrain({}));
    }
    ViewNode& child = node.ChildAt(0);
    const Size child_size = context.Measure(child, constraints.Loose());
    const float width = std::isfinite(constraints.max_width) ? constraints.max_width : child_size.width;
    const float height = std::isfinite(constraints.max_height) ? constraints.max_height : child_size.height;
    LayoutResult result;
    result.Place(child, {});
    return result.SetSize(constraints.Constrain({width, height}));
  }
};

void ScheduleSync(detail::SnapDragControllerState& state) {
  if(state.sync_scheduled || !state.alive || !*state.alive || !state.post_sync) return;
  state.sync_scheduled = true;
  state.post_sync();
}

struct SnapDragStageProbe {
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  std::shared_ptr<detail::SnapDragControllerState> state;

  bool operator==(const SnapDragStageProbe&) const = default;
};

class SnapDragStageProbe::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const SnapDragStageProbe& value) { Update(node, value); }

  ~Extension() override {
    if(state_) {
      state_->animate_to = {};
      state_->snap_immediate = {};
      state_->cancel_animation = {};
    }
  }

  void Update(ViewNode&, const SnapDragStageProbe& value) {
    if(state_ && state_ != value.state) {
      state_->animate_to = {};
      state_->snap_immediate = {};
      state_->cancel_animation = {};
      animating_ = false;
    }
    state_ = value.state;
    if(state_) {
      state_->animate_to = [this](Point from, Point velocity, Point to, const AnimationSpec& motion) {
        motion_x_.Seek(from.x, velocity.x);
        motion_y_.Seek(from.y, velocity.y);
        motion_x_.AnimateTo(to.x, motion);
        motion_y_.AnimateTo(to.y, motion);
        target_ = to;
        animating_ = true;
        InvalidatePaint(PaintInvalidation::Foreground);
      };
      state_->snap_immediate = [this](Point to) {
        animating_ = false;
        target_ = to;
        motion_x_.Set(to.x);
        motion_y_.Set(to.y);
      };
      state_->cancel_animation = [this]() { animating_ = false; };
      state_->needs_sync = true;
    }
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
    if(acc.changed || !running) {
      if(state_->position.IsValid() && state_->position.Get() != next) {
        state_->position = next;
      }
      if(state_->has_stage && state_->has_item && state_->normalized.IsValid()) {
        const TravelBounds bounds = ComputeTravelBounds(state_->stage_size, state_->item_size, state_->style.margin);
        const Point norm = PixelToNormalized(bounds, next);
        if(state_->normalized.Get() != norm) {
          state_->normalized = norm;
        }
      }
    }
    if(!running) {
      animating_ = false;
      return {};
    }
    return acc.Result();
  }

  PaintInvalidation PrepareGeometry(ViewNode& node, TextMeasurer&) override {
    if(!state_) return PaintInvalidation::None;
    const Rect b = node.Bounds();
    const Size size{b.width, b.height};
    const bool valid = size.width > 0.0F || size.height > 0.0F;
    const bool changed = state_->has_stage != valid || state_->stage_size != size;
    if(changed) {
      state_->has_stage = valid;
      state_->stage_size = size;
    }
    if(changed || state_->needs_sync) {
      state_->needs_sync = false;
      ScheduleSync(*state_);
    }
    return PaintInvalidation::None;
  }

private:
  std::shared_ptr<detail::SnapDragControllerState> state_;
  MotionController motion_x_{0.0F};
  MotionController motion_y_{0.0F};
  Point target_{};
  bool animating_ = false;
};

const huxerui::detail::ModifierDescriptor& SnapDragStageProbe::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<SnapDragStageProbe, SnapDragStageProbe::Extension>();
}

struct SnapDragItemProbe {
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  std::shared_ptr<detail::SnapDragControllerState> state;

  bool operator==(const SnapDragItemProbe&) const = default;
};

class SnapDragItemProbe::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const SnapDragItemProbe& value) { Update(node, value); }

  void Update(ViewNode&, const SnapDragItemProbe& value) { state_ = value.state; }

  PaintInvalidation PrepareGeometry(ViewNode& node, TextMeasurer&) override {
    if(!state_) return PaintInvalidation::None;
    const Rect b = node.Bounds();
    const Size size{b.width, b.height};
    const bool valid = size.width > 0.0F || size.height > 0.0F;
    if(state_->has_item != valid || state_->item_size != size) {
      state_->has_item = valid;
      state_->item_size = size;
      ScheduleSync(*state_);
    }
    return PaintInvalidation::None;
  }

private:
  std::shared_ptr<detail::SnapDragControllerState> state_;
};

const huxerui::detail::ModifierDescriptor& SnapDragItemProbe::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<SnapDragItemProbe, SnapDragItemProbe::Extension>();
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

} // namespace

bool SnapDragController::IsDragging() const {
  if(!state_ || !state_->dragging.IsValid()) return false;
  return state_->dragging.Get();
}

Point SnapDragController::Position() const {
  if(!state_ || !state_->position.IsValid()) return {};
  return state_->position.Get();
}

Point SnapDragController::NormalizedPosition() const {
  if(!state_ || !state_->normalized.IsValid()) return {};
  return state_->normalized.Get();
}

std::size_t SnapDragController::ActiveAnchor() const {
  if(!state_ || !state_->active_anchor.IsValid()) return 0;
  return state_->active_anchor.Get();
}

bool SnapDragController::SnapToAnchor(std::size_t index, bool animated) const {
  if(!state_ || !state_->has_stage || !state_->has_item) return false;
  const Point fallback = state_->normalized.IsValid() ? state_->normalized.Get() : state_->rest_normalized;
  const std::optional<Point> target_norm = AnchorToNormalized(state_->style, index, fallback);
  if(!target_norm.has_value()) return false;
  if(state_->active_anchor.IsValid() && state_->active_anchor.Get() != index) {
    state_->active_anchor = index;
  }
  state_->rest_normalized = *target_norm;
  const TravelBounds bounds = ComputeTravelBounds(state_->stage_size, state_->item_size, state_->style.margin);
  const Point target_px = NormalizedToPixel(bounds, *target_norm);
  const Point current = state_->position.IsValid() ? state_->position.Get() : Point{};
  if(animated && state_->animate_to) {
    state_->animate_to(current, {}, target_px, state_->style.snap_motion);
  } else {
    if(state_->snap_immediate) state_->snap_immediate(target_px);
    if(state_->position.IsValid()) state_->position = target_px;
    if(state_->normalized.IsValid()) state_->normalized = *target_norm;
  }
  return true;
}

bool SnapDragController::SnapToNormalized(Point normalized, bool animated) const {
  if(!state_ || !state_->has_stage || !state_->has_item) return false;
  if(!std::isfinite(normalized.x) || !std::isfinite(normalized.y)) {
    throw std::invalid_argument("HuxerUI Interaction: SnapDrag normalized coordinates must be finite");
  }
  const Point clamped{std::clamp(normalized.x, 0.0F, 1.0F), std::clamp(normalized.y, 0.0F, 1.0F)};
  state_->rest_normalized = clamped;
  const TravelBounds bounds = ComputeTravelBounds(state_->stage_size, state_->item_size, state_->style.margin);
  const Point target_px = NormalizedToPixel(bounds, clamped);
  const Point current = state_->position.IsValid() ? state_->position.Get() : Point{};
  if(animated && state_->animate_to) {
    state_->animate_to(current, {}, target_px, state_->style.snap_motion);
  } else {
    if(state_->snap_immediate) state_->snap_immediate(target_px);
    if(state_->position.IsValid()) state_->position = target_px;
    if(state_->normalized.IsValid()) state_->normalized = clamped;
  }
  return true;
}

SnapDragController UseSnapDragController(std::size_t initial_anchor) {
  auto dragging = UseState(false);
  auto position = UseState(Point{});
  auto normalized = UseState(Point{});
  auto active_anchor = UseState(initial_anchor);
  auto holder = UseState(std::make_shared<detail::SnapDragControllerState>());
  auto shared = holder.Get();
  shared->dragging = dragging;
  shared->position = position;
  shared->normalized = normalized;
  shared->active_anchor = active_anchor;
  return SnapDragController(std::move(shared));
}

View SnapDrag(std::function<View(SnapDragSnapshot)> builder, SnapDragStyle style,
              std::function<void(std::size_t anchor, Point position)> on_snapped) {
  ValidateStyle(style);
  if(!builder) {
    throw std::invalid_argument("HuxerUI Interaction: SnapDrag requires a view builder");
  }
  return Scope([builder = std::move(builder), style = std::move(style), on_snapped = std::move(on_snapped)]() -> View {
    TaskScope tasks = UseTaskScope();
    auto alive = UseState(std::make_shared<bool>(true)).Get();
    auto local_dragging = UseState(false);
    auto local_pos = UseState(Point{});
    auto local_norm = UseState(Point{});
    auto local_anchor = UseState(style.initial_anchor);
    auto local_holder = UseState(std::make_shared<detail::SnapDragControllerState>());

    std::shared_ptr<detail::SnapDragControllerState> state =
        style.controller.state_ ? style.controller.state_ : local_holder.Get();
    if(!style.controller.state_) {
      state->dragging = local_dragging;
      state->position = local_pos;
      state->normalized = local_norm;
      state->active_anchor = local_anchor;
    }
    state->style = style;
    state->style.controller = {};
    state->on_snapped = on_snapped;
    state->alive = alive;
    *alive = true;

    std::weak_ptr<detail::SnapDragControllerState> weak_state = state;
    state->post_sync = [tasks, weak_state, alive]() {
      tasks.Post([weak_state, alive]() {
        if(!*alive) return;
        auto locked = weak_state.lock();
        if(!locked) return;
        locked->sync_scheduled = false;
        if(!locked->has_stage || !locked->has_item) return;
        if(locked->dragging.IsValid() && locked->dragging.Get()) return;

        const TravelBounds bounds = ComputeTravelBounds(locked->stage_size, locked->item_size, locked->style.margin);
        if(!locked->positioned_once) {
          std::size_t init_idx = locked->active_anchor.IsValid() ? locked->active_anchor.Get() : 0U;
          if(init_idx == 0 && locked->style.initial_anchor != 0) {
            init_idx = locked->style.initial_anchor;
          }
          const std::optional<Point> norm = AnchorToNormalized(locked->style, init_idx, locked->rest_normalized);
          const Point resolved_norm = norm.value_or(Point{0.0F, 0.0F});
          const std::size_t resolved_idx = norm.has_value() ? init_idx : 0U;
          const Point target_px = NormalizedToPixel(bounds, resolved_norm);
          locked->positioned_once = true;
          locked->rest_normalized = resolved_norm;
          if(locked->active_anchor.IsValid() && locked->active_anchor.Get() != resolved_idx) {
            locked->active_anchor = resolved_idx;
          }
          if(locked->normalized.IsValid()) locked->normalized = resolved_norm;
          if(locked->snap_immediate) locked->snap_immediate(target_px);
          if(locked->position.IsValid() && locked->position.Get() != target_px) {
            locked->position = target_px;
          }
          return;
        }
        const Point projected_px = NormalizedToPixel(bounds, locked->rest_normalized);
        if(locked->position.IsValid()) {
          const Point cur = locked->position.Get();
          if(std::abs(cur.x - projected_px.x) > 0.5F || std::abs(cur.y - projected_px.y) > 0.5F) {
            if(locked->snap_immediate) locked->snap_immediate(projected_px);
            locked->position = projected_px;
            if(locked->normalized.IsValid()) locked->normalized = locked->rest_normalized;
          }
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

    const bool is_dragging = state->dragging.IsValid() ? state->dragging.Get() : false;
    const Point current_pos = state->position.IsValid() ? state->position.Get() : Point{};
    const SnapDragSnapshot snapshot{
        .dragging = is_dragging,
        .active_anchor = state->active_anchor.IsValid() ? state->active_anchor.Get() : 0U,
        .position = current_pos,
        .normalized = state->normalized.IsValid() ? state->normalized.Get() : Point{},
    };

    View item = Stack{builder(snapshot)}
                    .With(Offset(current_pos))
                    .With(Scale(AnimateTo(is_dragging ? style.drag_scale : 1.0F,
                                          TweenSpec{.duration = 0.14, .easing = Easing::EaseOut})))
                    .With(PointerCursor{is_dragging ? PointerCursorKind::Grabbing : style.cursor})
                    .With(DragGesture{})
                    .With(SnapDragItemProbe{.state = state})
                    .On<DragEvents::Started>([state](const DragEvent&) {
                      if(state->cancel_animation) state->cancel_animation();
                      state->drag_start_pos = state->position.IsValid() ? state->position.Get() : Point{};
                      state->pre_drag_normalized = state->rest_normalized;
                      state->pre_drag_anchor = state->active_anchor.IsValid() ? state->active_anchor.Get() : 0U;
                      if(state->dragging.IsValid()) state->dragging = true;
                    })
                    .On<DragEvents::Changed>([state](const DragEvent& event) {
                      if(!state->dragging.IsValid() || !state->dragging.Get()) return;
                      Point next = state->drag_start_pos + event.translation;
                      const TravelBounds bounds =
                          ComputeTravelBounds(state->stage_size, state->item_size, state->style.margin);
                      if(state->style.clamp_to_bounds && state->has_stage && state->has_item) {
                        next = bounds.Clamp(next);
                      }
                      if(state->position.IsValid()) state->position = next;
                      if(state->normalized.IsValid() && state->has_stage && state->has_item) {
                        state->normalized = PixelToNormalized(bounds, next);
                      }
                    })
                    .On<DragEvents::Ended>([state](const DragEvent& event) {
                      if(state->dragging.IsValid()) state->dragging = false;
                      const TravelBounds bounds =
                          ComputeTravelBounds(state->stage_size, state->item_size, state->style.margin);
                      Point current = state->drag_start_pos + event.translation;
                      if(state->style.clamp_to_bounds && state->has_stage && state->has_item) {
                        current = bounds.Clamp(current);
                      }
                      const Point vel = ClampVelocity(event.velocity, state->style.maximum_velocity);
                      const Point projected{
                          current.x + vel.x * state->style.projection_time,
                          current.y + vel.y * state->style.projection_time,
                      };
                      const SnapTarget target = ResolveNearestSnap(state->style, bounds, projected);
                      state->rest_normalized = target.normalized;
                      if(state->active_anchor.IsValid()) state->active_anchor = target.index;
                      if(state->animate_to) {
                        state->animate_to(current, vel, target.pixel, state->style.snap_motion);
                      } else {
                        if(state->position.IsValid()) state->position = target.pixel;
                        if(state->normalized.IsValid()) state->normalized = target.normalized;
                      }
                      if(state->on_snapped) {
                        state->on_snapped(target.index, target.pixel);
                      }
                    })
                    .On<DragEvents::Canceled>([state](const DragEvent&) {
                      if(state->dragging.IsValid()) state->dragging = false;
                      const TravelBounds bounds =
                          ComputeTravelBounds(state->stage_size, state->item_size, state->style.margin);
                      const Point back_px = NormalizedToPixel(bounds, state->pre_drag_normalized);
                      const Point current = state->position.IsValid() ? state->position.Get() : Point{};
                      state->rest_normalized = state->pre_drag_normalized;
                      if(state->active_anchor.IsValid()) state->active_anchor = state->pre_drag_anchor;
                      if(state->animate_to) {
                        state->animate_to(current, {}, back_px, state->style.snap_motion);
                      } else {
                        if(state->position.IsValid()) state->position = back_px;
                        if(state->normalized.IsValid()) state->normalized = state->pre_drag_normalized;
                      }
                    });

    return SnapDragStageLayout{std::move(item)}.With(SnapDragStageProbe{.state = state});
  });
}

View SnapDrag(View content, SnapDragStyle style, std::function<void(std::size_t anchor, Point position)> on_snapped) {
  ValidateStyle(style);
  return SnapDrag([content = std::move(content)](SnapDragSnapshot) -> View { return content; }, std::move(style),
                  std::move(on_snapped));
}

} // namespace huxerui::interaction
