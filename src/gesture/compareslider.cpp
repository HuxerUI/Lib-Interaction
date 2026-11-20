#include <huxerui/interaction/gesture/compareslider.h>

#include "../internal/validate.h"

#include <huxerui/layout.h>
#include <huxerui/lifecycle.h>
#include <huxerui/modifier.h>
#include <huxerui/paint.h>
#include <huxerui/semantics.h>
#include <huxerui/state.h>
#include <huxerui/task.h>
#include <huxerui/vector.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

namespace huxerui::interaction {

namespace detail {

struct CompareSliderControllerState {
  State<float> progress;
  State<bool> dragging;
  State<bool> hovered;
  State<bool> focused;
  CompareSliderStyle style;
  std::function<void(float)> on_change;

  bool initialized = false;
  bool has_explicit_controller_initial = false;
  bool sync_scheduled = false;
  bool needs_sync = false;
  bool enabled = true;

  std::shared_ptr<bool> alive;
  std::function<void()> post_sync;
  std::function<void(float target, bool animated)> set_progress_fn;
};

} // namespace detail

namespace {

constexpr float kProgressEpsilon = 1e-5F;
constexpr float kDragMoveSlop = 6.0F;
constexpr float kDoubleClickMaxDistance = 10.0F;
constexpr auto kDoubleClickInterval = std::chrono::duration<double>(0.35);

void ValidateStyle(const CompareSliderStyle& style) {
  internal::RequireUnitInterval(style.min_progress,
                                "HuxerUI Interaction: CompareSlider min_progress must be finite and in [0, 1]");
  internal::RequireUnitInterval(style.max_progress,
                                "HuxerUI Interaction: CompareSlider max_progress must be finite and in [0, 1]");
  if(style.min_progress > style.max_progress) {
    throw std::invalid_argument(
        "HuxerUI Interaction: CompareSlider min_progress must not exceed max_progress");
  }
  internal::RequireFinite(style.initial_progress,
                          "HuxerUI Interaction: CompareSlider initial_progress must be finite");
  if(style.initial_progress < style.min_progress || style.initial_progress > style.max_progress) {
    throw std::invalid_argument(
        "HuxerUI Interaction: CompareSlider initial_progress must lie within [min_progress, max_progress]");
  }
  internal::RequirePositive(style.handle_hit_extent,
                            "HuxerUI Interaction: CompareSlider handle_hit_extent must be finite and positive");
  internal::RequireFinite(style.keyboard_step,
                          "HuxerUI Interaction: CompareSlider keyboard_step must be finite and in (0, 1]");
  if(style.keyboard_step <= 0.0F || style.keyboard_step > 1.0F) {
    throw std::invalid_argument(
        "HuxerUI Interaction: CompareSlider keyboard_step must be finite and in (0, 1]");
  }
  internal::RequireNonNegative(style.divider_thickness,
                               "HuxerUI Interaction: CompareSlider divider_thickness must be finite and non-negative");
  internal::RequireNonNegative(style.handle_size,
                               "HuxerUI Interaction: CompareSlider handle_size must be finite and non-negative");
  if(const auto* tween = std::get_if<TweenSpec>(&style.motion)) {
    internal::RequireNonNegative(
        tween->duration, "HuxerUI Interaction: CompareSlider motion tween duration must be finite and non-negative");
  } else if(const auto* spring = std::get_if<SpringSpec>(&style.motion)) {
    internal::RequirePositive(
        spring->stiffness,
        "HuxerUI Interaction: CompareSlider motion spring stiffness must be finite and positive");
    internal::RequirePositive(
        spring->damping_ratio,
        "HuxerUI Interaction: CompareSlider motion spring damping ratio must be finite and positive");
  }
}

float ClampToStyle(const CompareSliderStyle& style, float value) noexcept {
  if(!std::isfinite(value)) return style.initial_progress;
  return std::clamp(value, style.min_progress, style.max_progress);
}

void ScheduleSync(detail::CompareSliderControllerState& state) {
  if(state.sync_scheduled || !state.alive || !*state.alive || !state.post_sync) return;
  state.sync_scheduled = true;
  state.post_sync();
}

struct CompareClipData {
  Axis axis = Axis::Horizontal;
  float progress = 0.5F;

  bool operator==(const CompareClipData&) const = default;
};

struct CompareClipLayoutValue {
  using Value = CompareClipData;
};

class CompareBeforeClipLayout final : public Layout<CompareBeforeClipLayout> {
public:
  using Layout::Layout;

  static LayoutResult Measure(LayoutContext& context, ViewNode& node, const Constraints& constraints) {
    LayoutResult result;
    if(node.ChildCount() == 0U) {
      return result.SetSize(constraints.Constrain(Size{}));
    }
    const CompareClipData data = node.LayoutValueOr<CompareClipLayoutValue>(CompareClipData{});
    const bool finite_w = std::isfinite(constraints.max_width);
    const bool finite_h = std::isfinite(constraints.max_height);
    const Constraints child_constraints{
        finite_w ? constraints.max_width : constraints.min_width,
        constraints.max_width,
        finite_h ? constraints.max_height : constraints.min_height,
        constraints.max_height,
    };
    const Size before_size = context.Measure(node.ChildAt(0U), child_constraints);
    result.Place(node.ChildAt(0U), Point{0.0F, 0.0F});

    const float full_w = finite_w ? constraints.max_width : before_size.width;
    const float full_h = finite_h ? constraints.max_height : before_size.height;
    const float progress = std::clamp(data.progress, 0.0F, 1.0F);
    const Size clipped = data.axis == Axis::Horizontal ? Size{std::clamp(full_w * progress, 0.0F, full_w), full_h}
                                                       : Size{full_w, std::clamp(full_h * progress, 0.0F, full_h)};
    return result.SetSize(constraints.Constrain(clipped));
  }
};

struct CompareStageData {
  Axis axis = Axis::Horizontal;
  float progress = 0.5F;
  bool has_custom_handle = false;
  bool has_divider = false;
  float divider_thickness = 2.0F;

  bool operator==(const CompareStageData&) const = default;
};

struct CompareStageLayoutValue {
  using Value = CompareStageData;
};

class CompareSliderStageLayout final : public Layout<CompareSliderStageLayout> {
public:
  using Layout::Layout;

  static LayoutResult Measure(LayoutContext& context, ViewNode& node, const Constraints& constraints) {
    LayoutResult result;
    if(node.ChildCount() == 0U) {
      return result.SetSize(constraints.Constrain(Size{}));
    }

    const CompareStageData data = node.LayoutValueOr<CompareStageLayoutValue>(CompareStageData{});

    // Measure `after` (Child 0) under stage constraints.
    const Size after_size = context.Measure(node.ChildAt(0U), constraints);
    Size stage_size = constraints.Constrain(after_size);

    // If loose constraints and `after` measured zero along an axis, probe `before` (Child 1) so empty `after`
    // placeholders still inherit intrinsic dimensions cleanly.
    if(node.ChildCount() >= 2U && (stage_size.width <= 0.0F || stage_size.height <= 0.0F)) {
      const Size before_probe = context.Measure(node.ChildAt(1U), constraints);
      if(stage_size.width <= 0.0F && before_probe.width > 0.0F) {
        stage_size.width = constraints.Constrain({before_probe.width, stage_size.height}).width;
      }
      if(stage_size.height <= 0.0F && before_probe.height > 0.0F) {
        stage_size.height = constraints.Constrain({stage_size.width, before_probe.height}).height;
      }
      if(stage_size.width > 0.0F && stage_size.height > 0.0F
         && (after_size.width != stage_size.width || after_size.height != stage_size.height)) {
        (void)context.Measure(
            node.ChildAt(0U),
            Constraints{stage_size.width, stage_size.width, stage_size.height, stage_size.height});
      }
    }

    result.Place(node.ChildAt(0U), Point{0.0F, 0.0F});

    if(node.ChildCount() >= 2U) {
      const Constraints clip_constraints{0.0F, stage_size.width, 0.0F, stage_size.height};
      (void)context.Measure(node.ChildAt(1U), clip_constraints);
      result.Place(node.ChildAt(1U), Point{0.0F, 0.0F});
    }

    if(data.has_custom_handle) {
      const float progress = std::clamp(data.progress, 0.0F, 1.0F);
      const float split_x = stage_size.width * progress;
      const float split_y = stage_size.height * progress;
      std::size_t handle_idx = 2U;

      if(data.has_divider && node.ChildCount() > 3U) {
        const float t = std::max(0.0F, data.divider_thickness);
        const Constraints div_constraints =
            data.axis == Axis::Horizontal
                ? Constraints{t, t, stage_size.height, stage_size.height}
                : Constraints{stage_size.width, stage_size.width, t, t};
        const Size div_size = context.Measure(node.ChildAt(2U), div_constraints);
        const Point div_pos = data.axis == Axis::Horizontal ? Point{split_x - div_size.width * 0.5F, 0.0F}
                                                            : Point{0.0F, split_y - div_size.height * 0.5F};
        result.Place(node.ChildAt(2U), div_pos);
        handle_idx = 3U;
      }

      if(node.ChildCount() > handle_idx) {
        const Constraints handle_constraints{0.0F, stage_size.width, 0.0F, stage_size.height};
        const Size handle_size = context.Measure(node.ChildAt(handle_idx), handle_constraints);
        const Point center = data.axis == Axis::Horizontal ? Point{split_x, stage_size.height * 0.5F}
                                                           : Point{stage_size.width * 0.5F, split_y};
        result.Place(node.ChildAt(handle_idx),
                     Point{center.x - handle_size.width * 0.5F, center.y - handle_size.height * 0.5F});
      }
    }

    return result.SetSize(stage_size);
  }
};

struct CompareSliderModifier {
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  std::shared_ptr<detail::CompareSliderControllerState> state;
  TaskScope tasks{};
  float progress = 0.5F;
  Axis axis = Axis::Horizontal;
  float min_progress = 0.0F;
  float max_progress = 1.0F;
  float keyboard_step = 0.05F;
  bool has_custom_handle = false;

  bool operator==(const CompareSliderModifier& other) const noexcept {
    return state == other.state && progress == other.progress && axis == other.axis
           && min_progress == other.min_progress && max_progress == other.max_progress
           && keyboard_step == other.keyboard_step && has_custom_handle == other.has_custom_handle;
  }
};

class CompareSliderModifier::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const CompareSliderModifier& value) { Update(node, value); }

  ~Extension() override {
    ++click_token_;
    if(state_) {
      state_->set_progress_fn = {};
    }
  }

  void Update(ViewNode&, const CompareSliderModifier& value) {
    const bool state_changed = state_ != value.state;
    if(state_ && state_changed) {
      state_->set_progress_fn = {};
      animating_ = false;
      pointer_id_.reset();
      ++click_token_;
      pending_click_ = false;
    }
    state_ = value.state;
    tasks_ = value.tasks;
    has_custom_handle_ = value.has_custom_handle;
    if(state_) {
      state_->set_progress_fn = [this](float target, bool animated) { StartSetProgress(target, animated); };
      if(state_changed) {
        const float initial = state_->progress.IsValid() ? state_->progress.Get() : value.progress;
        motion_.Set(initial);
        anim_target_ = initial;
        state_->needs_sync = true;
      }
    }
    InvalidatePaint(PaintInvalidation::Foreground);
  }

  [[nodiscard]] bool HitTest(ViewNode& node, Point position) const override {
    if(!node.IsEnabled() || !state_) return false;
    const Rect bounds = node.Bounds();
    if(!bounds.Contains(position)) return false;
    if(state_->style.drag_anywhere) return true;
    const float progress = CurrentProgress();
    if(state_->style.axis == Axis::Horizontal) {
      const float split_x = bounds.x + bounds.width * progress;
      return std::abs(position.x - split_x) <= state_->style.handle_hit_extent * 0.5F;
    }
    const float split_y = bounds.y + bounds.height * progress;
    return std::abs(position.y - split_y) <= state_->style.handle_hit_extent * 0.5F;
  }

  [[nodiscard]] bool HoverHitTest(ViewNode& node, Point position) const override {
    return HitTest(node, position);
  }

  void OnHover(ViewNode&, const HoverEvent& event) override {
    if(!state_) return;
    const bool is_hovered = event.type != HoverEventType::Leave;
    if(state_->hovered.IsValid() && state_->hovered.Get() != is_hovered) {
      state_->hovered = is_hovered;
    }
    InvalidatePaint(PaintInvalidation::Foreground);
  }

  void OnFocusChanged(ViewNode&, bool focused, bool) override {
    if(!state_) return;
    if(state_->focused.IsValid() && state_->focused.Get() != focused) {
      state_->focused = focused;
    }
    InvalidatePaint(PaintInvalidation::Foreground);
  }

  void OnInteraction(ViewNode&, const InteractionState& state, const std::optional<InteractionEvent>&) override {
    if(!state_) return;
    state_->enabled = state.enabled;
    if(!state.enabled && pointer_id_.has_value()) {
      pointer_id_.reset();
      ++click_token_;
      pending_click_ = false;
      state_->needs_sync = true;
      ScheduleSync(*state_);
    }
  }

  PointerResult OnPointer(ViewNode& node, const PointerEvent& event) override {
    if(!state_ || !node.IsEnabled() || !state_->enabled) {
      if(pointer_id_.has_value()) {
        CancelActiveDrag(false);
      }
      return PointerResult::Ignored;
    }

    if(event.type == PointerEventType::Down) {
      if(event.changed_button != PointerButton::Primary || pointer_id_.has_value()) {
        return PointerResult::Ignored;
      }
      pointer_id_ = event.pointer_id;
      animating_ = false;
      down_position_ = event.position;
      drag_start_progress_ = CurrentProgress();
      moved_during_press_ = false;
      if(state_->dragging.IsValid() && !state_->dragging.Get()) {
        state_->dragging = true;
      }
      ApplyPointerPosition(node, event.position);
      InvalidatePaint(PaintInvalidation::Foreground);
      return PointerResult::Capture;
    }

    if(!pointer_id_.has_value() || *pointer_id_ != event.pointer_id) {
      return PointerResult::Ignored;
    }

    if(event.type == PointerEventType::Move) {
      if(std::hypot(event.position.x - down_position_.x, event.position.y - down_position_.y) > kDragMoveSlop) {
        moved_during_press_ = true;
        ++click_token_;
        pending_click_ = false;
      }
      ApplyPointerPosition(node, event.position);
      InvalidatePaint(PaintInvalidation::Foreground);
      return PointerResult::Handled;
    }

    if(event.type == PointerEventType::Up) {
      if(std::hypot(event.position.x - down_position_.x, event.position.y - down_position_.y) > kDragMoveSlop) {
        moved_during_press_ = true;
      }
      ApplyPointerPosition(node, event.position);
      const bool clean_click = !moved_during_press_;
      pointer_id_.reset();
      if(state_->dragging.IsValid() && state_->dragging.Get()) {
        state_->dragging = false;
      }
      if(clean_click && state_->style.double_click_resets) {
        HandleCleanClick(event.position);
      } else {
        ++click_token_;
        pending_click_ = false;
      }
      InvalidatePaint(PaintInvalidation::Foreground);
      return PointerResult::Handled;
    }

    if(event.type == PointerEventType::Cancel) {
      CancelActiveDrag(true);
      InvalidatePaint(PaintInvalidation::Foreground);
      return PointerResult::Handled;
    }

    return PointerResult::Ignored;
  }

  [[nodiscard]] bool OnKey(ViewNode& node, const KeyEvent& event) override {
    if(!state_ || !node.IsEnabled() || !state_->enabled || !state_->style.keyboard_focusable
       || event.type != KeyEventType::Down) {
      return false;
    }
    if(event.key == Key::Escape && pointer_id_.has_value()) {
      CancelActiveDrag(true);
      InvalidatePaint(PaintInvalidation::Foreground);
      return true;
    }
    if(pointer_id_.has_value() || event.modifiers.alt || event.modifiers.control || event.modifiers.meta) {
      return false;
    }

    const float base = state_->style.keyboard_step;
    const float fine = event.modifiers.shift ? (base * 0.2F) : base;
    const float page = std::min(1.0F, base * 5.0F);
    const bool horizontal = state_->style.axis == Axis::Horizontal;
    const float current = CurrentProgress();

    std::optional<float> next;
    switch(event.key) {
    case Key::ArrowLeft:
      next = horizontal ? (current - fine) : (current - fine);
      break;
    case Key::ArrowRight:
      next = horizontal ? (current + fine) : (current + fine);
      break;
    case Key::ArrowUp:
      next = horizontal ? (current + fine) : (current - fine);
      break;
    case Key::ArrowDown:
      next = horizontal ? (current - fine) : (current + fine);
      break;
    case Key::PageUp:
      next = horizontal ? (current + page) : (current - page);
      break;
    case Key::PageDown:
      next = horizontal ? (current - page) : (current + page);
      break;
    case Key::Home:
      next = state_->style.min_progress;
      break;
    case Key::End:
      next = state_->style.max_progress;
      break;
    default:
      return false;
    }
    if(!next.has_value()) return false;
    animating_ = false;
    const float clamped = ClampToStyle(state_->style, *next);
    motion_.Set(clamped);
    ApplyProgress(clamped, true);
    InvalidatePaint(PaintInvalidation::Foreground);
    return true;
  }

  void BuildSemantics(SemanticBuilder& builder) const override {
    if(!state_) return;
    builder.SetOwner(Semantics{
        .role = SemanticRole::Slider,
        .range = SemanticRange{
            .minimum = static_cast<double>(state_->style.min_progress),
            .maximum = static_cast<double>(state_->style.max_progress),
            .current = static_cast<double>(ClampToStyle(state_->style, CurrentProgress())),
            .step = static_cast<double>(state_->style.keyboard_step),
        },
    });
    builder.AddAction(0, SemanticActionKind::Increment);
    builder.AddAction(0, SemanticActionKind::Decrement);
  }

  [[nodiscard]] bool OnSemanticAction(std::uint64_t local_id, const SemanticAction& action) override {
    if(local_id != 0 || !state_ || !state_->enabled) return false;
    const float current = CurrentProgress();
    if(action.kind == SemanticActionKind::Increment) {
      animating_ = false;
      const float clamped = ClampToStyle(state_->style, current + state_->style.keyboard_step);
      motion_.Set(clamped);
      ApplyProgress(clamped, true);
      InvalidatePaint(PaintInvalidation::Foreground);
      return true;
    }
    if(action.kind == SemanticActionKind::Decrement) {
      animating_ = false;
      const float clamped = ClampToStyle(state_->style, current - state_->style.keyboard_step);
      motion_.Set(clamped);
      ApplyProgress(clamped, true);
      InvalidatePaint(PaintInvalidation::Foreground);
      return true;
    }
    return false;
  }

  FrameResult OnFrame(ViewNode&, const FrameInfo& frame) override {
    if(!animating_ || !state_) return {};
    if(frame.reduced_motion) {
      animating_ = false;
      motion_.Set(anim_target_);
      ApplyProgress(anim_target_, true);
      InvalidatePaint(PaintInvalidation::Foreground);
      return {};
    }
    const MotionAdvanceResult res = motion_.Advance(frame);
    const bool running = motion_.IsRunning();
    const float value = running ? ClampToStyle(state_->style, motion_.Value()) : anim_target_;
    if(res.changed || !running) {
      ApplyProgress(value, true);
      InvalidatePaint(PaintInvalidation::Foreground);
    }
    if(!running) {
      animating_ = false;
      return {};
    }
    return {res.needs_frame, res.wake_after};
  }

  PaintInvalidation PrepareGeometry(ViewNode&, TextMeasurer&) override {
    if(!state_) return PaintInvalidation::None;
    const float cur = CurrentProgress();
    const float clamped = ClampToStyle(state_->style, cur);
    if(state_->needs_sync || !state_->initialized || std::abs(cur - clamped) > kProgressEpsilon) {
      state_->needs_sync = false;
      ScheduleSync(*state_);
    }
    return PaintInvalidation::None;
  }

  void PaintAboveContent(const ViewNode& node, PaintContext& context) const override {
    if(has_custom_handle_ || !state_) return;
    const Rect frame = node.Bounds();
    if(frame.width <= 0.0F || frame.height <= 0.0F) return;

    const CompareSliderStyle& style = state_->style;
    const bool horizontal = style.axis == Axis::Horizontal;
    const float progress = std::clamp(CurrentProgress(), 0.0F, 1.0F);
    const float split = horizontal ? (frame.x + frame.width * progress) : (frame.y + frame.height * progress);

    if(style.divider_thickness > 0.0F && style.divider_color.alpha > 0.0F) {
      const Rect line = horizontal
                            ? Rect{split - style.divider_thickness * 0.5F, frame.y, style.divider_thickness,
                                   frame.height}
                            : Rect{frame.x, split - style.divider_thickness * 0.5F, frame.width,
                                   style.divider_thickness};
      context.DrawRect(line, style.divider_color);
    }

    if(style.handle_size <= 0.0F) return;

    const Point center = horizontal ? Point{split, frame.y + frame.height * 0.5F}
                                    : Point{frame.x + frame.width * 0.5F, split};
    const float r = style.handle_size * 0.5F;
    const bool hovered = state_->hovered.IsValid() && state_->hovered.Get();
    const bool focused = state_->focused.IsValid() && state_->focused.Get();
    const bool dragging = state_->dragging.IsValid() && state_->dragging.Get();

    if((hovered || focused || dragging) && style.handle_accent_color.alpha > 0.0F) {
      const Color halo{
          style.handle_accent_color.red,
          style.handle_accent_color.green,
          style.handle_accent_color.blue,
          dragging ? 0.30F : 0.20F,
      };
      context.DrawCircle(center, r + 4.0F, halo);
    }

    if(style.handle_color.alpha > 0.0F) {
      context.DrawCircle(center, r, style.handle_color);
    }

    if(r >= 6.0F && style.handle_accent_color.alpha > 0.0F) {
      const float dx = r * 0.24F;
      const float tip = r * 0.58F;
      const float hh = r * 0.28F;
      if(horizontal) {
        Path left_arrow;
        left_arrow.MoveTo({center.x - tip, center.y})
            .LineTo({center.x - dx, center.y - hh})
            .LineTo({center.x - dx, center.y + hh})
            .Close();
        context.FillPath(std::move(left_arrow), style.handle_accent_color);

        Path right_arrow;
        right_arrow.MoveTo({center.x + tip, center.y})
            .LineTo({center.x + dx, center.y + hh})
            .LineTo({center.x + dx, center.y - hh})
            .Close();
        context.FillPath(std::move(right_arrow), style.handle_accent_color);
      } else {
        Path up_arrow;
        up_arrow.MoveTo({center.x, center.y - tip})
            .LineTo({center.x + hh, center.y - dx})
            .LineTo({center.x - hh, center.y - dx})
            .Close();
        context.FillPath(std::move(up_arrow), style.handle_accent_color);

        Path down_arrow;
        down_arrow.MoveTo({center.x, center.y + tip})
            .LineTo({center.x - hh, center.y + dx})
            .LineTo({center.x + hh, center.y + dx})
            .Close();
        context.FillPath(std::move(down_arrow), style.handle_accent_color);
      }
    }
  }

private:
  [[nodiscard]] float CurrentProgress() const {
    if(!state_) return 0.5F;
    if(state_->progress.IsValid()) return state_->progress.Get();
    return state_->style.initial_progress;
  }

  void ApplyPointerPosition(const ViewNode& node, Point position) {
    if(!state_) return;
    const Rect frame = node.Bounds();
    const bool horizontal = state_->style.axis == Axis::Horizontal;
    const float span = horizontal ? frame.width : frame.height;
    if(span <= 0.0F) return;
    const float raw = horizontal ? ((position.x - frame.x) / span) : ((position.y - frame.y) / span);
    const float clamped = ClampToStyle(state_->style, raw);
    motion_.Set(clamped);
    ApplyProgress(clamped, true);
  }

  void ApplyProgress(float next, bool notify) {
    if(!state_ || !state_->progress.IsValid()) return;
    const float clamped = ClampToStyle(state_->style, next);
    const float prev = state_->progress.Get();
    if(std::abs(clamped - prev) <= kProgressEpsilon && clamped == prev) return;
    state_->progress = clamped;
    if(notify && state_->on_change && std::abs(clamped - prev) > kProgressEpsilon) {
      auto cb = state_->on_change;
      cb(clamped);
    }
  }

  void CancelActiveDrag(bool restore_progress) {
    pointer_id_.reset();
    ++click_token_;
    pending_click_ = false;
    if(!state_) return;
    if(state_->dragging.IsValid() && state_->dragging.Get()) {
      state_->dragging = false;
    }
    if(restore_progress) {
      const float clamped = ClampToStyle(state_->style, drag_start_progress_);
      motion_.Set(clamped);
      ApplyProgress(clamped, true);
    }
  }

  void StartSetProgress(float target, bool animated) {
    if(!state_) return;
    const float clamped = ClampToStyle(state_->style, target);
    const float current = CurrentProgress();
    const bool zero_tween = std::holds_alternative<TweenSpec>(state_->style.motion)
                            && std::get<TweenSpec>(state_->style.motion).duration <= 0.0;
    if(!animated || zero_tween || std::abs(clamped - current) <= kProgressEpsilon) {
      animating_ = false;
      motion_.Set(clamped);
      ApplyProgress(clamped, true);
      InvalidatePaint(PaintInvalidation::Foreground);
      return;
    }
    motion_.Set(current);
    motion_.AnimateTo(clamped, state_->style.motion);
    anim_target_ = clamped;
    animating_ = true;
    InvalidatePaint(PaintInvalidation::Foreground);
  }

  void HandleCleanClick(Point position) {
    if(!state_) return;
    if(pending_click_
       && std::hypot(position.x - pending_click_pos_.x, position.y - pending_click_pos_.y) <= kDoubleClickMaxDistance) {
      pending_click_ = false;
      ++click_token_;
      StartSetProgress(state_->style.initial_progress, true);
      return;
    }
    pending_click_ = true;
    pending_click_pos_ = position;
    const std::uint64_t token = ++click_token_;
    const std::shared_ptr<bool> alive = state_->alive;
    std::weak_ptr<detail::CompareSliderControllerState> weak_state = state_;
    tasks_.Launch([this, alive, weak_state, token]() -> Task<void> {
      co_await Delay(kDoubleClickInterval);
      if(!alive || !*alive || weak_state.expired() || click_token_ != token) {
        co_return;
      }
      pending_click_ = false;
    });
  }

  std::shared_ptr<detail::CompareSliderControllerState> state_;
  TaskScope tasks_{};
  MotionController motion_{0.5F};
  float anim_target_ = 0.5F;
  bool animating_ = false;
  bool has_custom_handle_ = false;

  std::optional<std::int64_t> pointer_id_{};
  Point down_position_{};
  float drag_start_progress_ = 0.5F;
  bool moved_during_press_ = false;

  bool pending_click_ = false;
  Point pending_click_pos_{};
  std::uint64_t click_token_ = 0;
};

const huxerui::detail::ModifierDescriptor& CompareSliderModifier::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<CompareSliderModifier, CompareSliderModifier::Extension>();
}

View BuildCompareSliderImpl(View before, View after, std::function<View(CompareSliderSnapshot)> handle_builder,
                            CompareSliderStyle style, std::function<void(float progress)> on_change,
                            std::shared_ptr<detail::CompareSliderControllerState> external_state) {
  ValidateStyle(style);
  const bool custom_handle = static_cast<bool>(handle_builder);

  return Scope([before = std::move(before), after = std::move(after), handle_builder = std::move(handle_builder),
                style = std::move(style), on_change = std::move(on_change),
                external_state = std::move(external_state), custom_handle]() mutable -> View {
    TaskScope tasks = UseTaskScope();
    auto alive = UseState(std::make_shared<bool>(true)).Get();
    auto local_progress = UseState(style.initial_progress);
    auto local_dragging = UseState(false);
    auto local_hovered = UseState(false);
    auto local_focused = UseState(false);
    auto local_holder = UseState(std::make_shared<detail::CompareSliderControllerState>());

    std::shared_ptr<detail::CompareSliderControllerState> state =
        external_state ? external_state : local_holder.Get();
    if(!external_state) {
      state->progress = local_progress;
      state->dragging = local_dragging;
      state->hovered = local_hovered;
      state->focused = local_focused;
    }

    state->style = style;
    state->style.controller = {};
    state->on_change = on_change;
    state->alive = alive;
    *alive = true;

    std::weak_ptr<detail::CompareSliderControllerState> weak_state = state;
    state->post_sync = [tasks, weak_state, alive]() {
      tasks.Post([weak_state, alive]() {
        if(!*alive) return;
        auto locked = weak_state.lock();
        if(!locked) return;
        locked->sync_scheduled = false;

        if(!locked->enabled && locked->dragging.IsValid() && locked->dragging.Get()) {
          locked->dragging = false;
        }
        if(!locked->initialized) {
          locked->initialized = true;
          if(!locked->has_explicit_controller_initial && locked->progress.IsValid()) {
            const float target = ClampToStyle(locked->style, locked->style.initial_progress);
            if(std::abs(locked->progress.Get() - target) > kProgressEpsilon) {
              locked->progress = target;
            }
          }
        }
        if(locked->progress.IsValid()) {
          const float cur = locked->progress.Get();
          const float clamped = ClampToStyle(locked->style, cur);
          if(std::abs(cur - clamped) > kProgressEpsilon) {
            locked->progress = clamped;
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

    return Scope([before = std::move(before), after = std::move(after), handle_builder, state, tasks, custom_handle]() -> View {
      const float raw_progress = state->progress.IsValid() ? state->progress.Get() : state->style.initial_progress;
      const float progress = ClampToStyle(state->style, raw_progress);
      const bool dragging = state->dragging.IsValid() && state->dragging.Get();
      const bool hovered = state->hovered.IsValid() && state->hovered.Get();
      const bool focused = state->focused.IsValid() && state->focused.Get();

      View clipped_before = CompareBeforeClipLayout(before)
                                .LayoutValue<CompareClipLayoutValue>(CompareClipData{
                                    .axis = state->style.axis,
                                    .progress = progress,
                                })
                                .With(ClipChildren{});

      const bool has_divider =
          custom_handle && state->style.divider_thickness > 0.0F && state->style.divider_color.alpha > 0.0F;

      std::vector<View> stage_children;
      stage_children.reserve(custom_handle ? (has_divider ? 4U : 3U) : 2U);
      stage_children.push_back(after);
      stage_children.push_back(std::move(clipped_before));
      if(custom_handle) {
        if(has_divider) {
          stage_children.push_back(Spacer{}.With(Background{state->style.divider_color}));
        }
        stage_children.push_back(handle_builder(CompareSliderSnapshot{
            .progress = progress,
            .dragging = dragging,
            .hovered = hovered,
            .focused = focused,
            .axis = state->style.axis,
        }));
      }

      const PointerCursorKind cursor = state->style.axis == Axis::Horizontal ? PointerCursorKind::ResizeHorizontal
                                                                             : PointerCursorKind::ResizeVertical;

      return CompareSliderStageLayout(std::move(stage_children))
          .LayoutValue<CompareStageLayoutValue>(CompareStageData{
              .axis = state->style.axis,
              .progress = progress,
              .has_custom_handle = custom_handle,
              .has_divider = has_divider,
              .divider_thickness = state->style.divider_thickness,
          })
          .With(ClipChildren{}, Focusable{state->style.keyboard_focusable}, PointerCursor{cursor},
                CompareSliderModifier{
                    .state = state,
                    .tasks = tasks,
                    .progress = progress,
                    .axis = state->style.axis,
                    .min_progress = state->style.min_progress,
                    .max_progress = state->style.max_progress,
                    .keyboard_step = state->style.keyboard_step,
                    .has_custom_handle = custom_handle,
                });
    });
  });
}

} // namespace

float CompareSliderController::Progress() const {
  if(!state_ || !state_->progress.IsValid()) return 0.5F;
  return state_->progress.Get();
}

bool CompareSliderController::IsDragging() const {
  if(!state_ || !state_->dragging.IsValid()) return false;
  return state_->dragging.Get();
}

void CompareSliderController::SetProgress(float progress, bool animated) const {
  if(!state_) return;
  internal::RequireFinite(progress, "HuxerUI Interaction: CompareSlider SetProgress target must be finite");
  if(state_->set_progress_fn) {
    state_->set_progress_fn(progress, animated);
  } else if(state_->progress.IsValid()) {
    state_->progress = ClampToStyle(state_->style, progress);
  }
}

void CompareSliderController::Reset(bool animated) const {
  if(!state_) return;
  SetProgress(state_->style.initial_progress, animated);
}

CompareSliderController UseCompareSliderController(float initial_progress) {
  internal::RequireUnitInterval(initial_progress,
                                "HuxerUI Interaction: UseCompareSliderController initial progress must be in [0, 1]");
  auto progress = UseState(initial_progress);
  auto dragging = UseState(false);
  auto hovered = UseState(false);
  auto focused = UseState(false);
  auto holder = UseState(std::make_shared<detail::CompareSliderControllerState>());
  auto shared = holder.Get();
  shared->progress = progress;
  shared->dragging = dragging;
  shared->hovered = hovered;
  shared->focused = focused;
  shared->has_explicit_controller_initial = (initial_progress != 0.5F);
  return CompareSliderController(std::move(shared));
}

View CompareSlider(View before, View after, std::function<View(CompareSliderSnapshot)> handle_builder,
                   CompareSliderStyle style, std::function<void(float progress)> on_change) {
  if(!handle_builder) {
    throw std::invalid_argument("HuxerUI Interaction: CompareSlider requires a handle builder");
  }
  auto external = style.controller.state_;
  return BuildCompareSliderImpl(std::move(before), std::move(after), std::move(handle_builder), std::move(style),
                                std::move(on_change), std::move(external));
}

View CompareSlider(View before, View after, CompareSliderStyle style,
                   std::function<void(float progress)> on_change) {
  auto external = style.controller.state_;
  return BuildCompareSliderImpl(std::move(before), std::move(after), {}, std::move(style), std::move(on_change),
                                std::move(external));
}

} // namespace huxerui::interaction
