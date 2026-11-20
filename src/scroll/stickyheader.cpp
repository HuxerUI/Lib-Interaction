#include <huxerui/interaction/scroll/stickyheader.h>

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
#include <cstdint>
#include <limits>
#include <map>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace huxerui::interaction {

namespace detail {

struct StickyHeaderSectionEntry {
  float content_start = 0.0F;
  float content_end = 0.0F;
  std::uint64_t token = 0;
  bool has_bounds = false;
};

struct StickyHeaderControllerState {
  State<std::optional<std::size_t>> pinned_section;
  State<float> push_progress;
  State<std::size_t> section_count;
  State<std::uint64_t> geometry_version;
  StickyHeaderStyle style;
  std::function<void(std::optional<std::size_t>)> on_pinned_changed;

  Rect viewport_bounds{};
  bool has_viewport = false;
  std::map<std::size_t, StickyHeaderSectionEntry> sections;
  std::unordered_map<std::size_t, float> header_extents;
  std::uint64_t next_token = 1;

  std::optional<std::size_t> last_notified_pinned{};
  bool notified_initialized = false;
  bool sync_scheduled = false;
  bool needs_sync = false;
  bool layout_dirty = false;
  std::shared_ptr<bool> alive;
  std::function<void()> post_sync;
  std::function<void(float from, float to, const AnimationSpec& motion)> start_smooth_scroll;
  std::function<void()> cancel_smooth_scroll;
};

} // namespace detail

namespace {

constexpr float kGeometryEpsilon = 0.25F;
constexpr float kProgressEpsilon = 0.002F;

void ValidateStyle(const StickyHeaderStyle& style) {
  internal::RequireNonNegative(style.sticky_offset,
                               "HuxerUI Interaction: StickyHeader sticky offset must be finite and non-negative");
  internal::RequireNonNegative(style.section_spacing,
                               "HuxerUI Interaction: StickyHeader section spacing must be finite and non-negative");
  if(const auto* tween = std::get_if<TweenSpec>(&style.scroll_motion)) {
    internal::RequireNonNegative(
        tween->duration, "HuxerUI Interaction: StickyHeader scroll motion tween duration must be finite and non-negative");
  } else if(const auto* spring = std::get_if<SpringSpec>(&style.scroll_motion)) {
    internal::RequirePositive(
        spring->stiffness,
        "HuxerUI Interaction: StickyHeader scroll motion spring stiffness must be finite and positive");
    internal::RequirePositive(
        spring->damping_ratio,
        "HuxerUI Interaction: StickyHeader scroll motion spring damping ratio must be finite and positive");
  }
}

struct StickyHeaderEnvironment {
  std::shared_ptr<detail::StickyHeaderControllerState> state;

  static StickyHeaderEnvironment Default() { return {}; }

  bool operator==(const StickyHeaderEnvironment&) const = default;
};

struct StickyHeaderTokenHolder {
  std::uint64_t token = 0;
};

struct StickyEvaluation {
  bool pinned = false;
  std::size_t active_index = 0;
  float boundary_distance = std::numeric_limits<float>::infinity();
  float push_progress = 0.0F;
};

float CurrentScrollOffset(const detail::StickyHeaderControllerState& st, bool* has_connected_scroll = nullptr) {
  if(st.style.scroll_controller.has_value() && st.style.scroll_controller->IsConnected()) {
    const ScrollMetrics metrics = st.style.scroll_controller->Metrics();
    if(metrics.axis == st.style.axis) {
      if(has_connected_scroll != nullptr) *has_connected_scroll = true;
      return metrics.offset;
    }
  }
  if(has_connected_scroll != nullptr) *has_connected_scroll = false;
  return 0.0F;
}

StickyEvaluation EvaluateSticky(const detail::StickyHeaderControllerState& st, float scroll_offset,
                                bool has_scroll_controller) {
  StickyEvaluation result;
  if(!st.has_viewport || st.sections.empty()) {
    return result;
  }

  struct SortedEntry {
    std::size_t index = 0;
    float start = 0.0F;
    float end = 0.0F;
  };
  std::vector<SortedEntry> sorted;
  sorted.reserve(st.sections.size());
  for(const auto& [index, entry] : st.sections) {
    if(!entry.has_bounds) continue;
    sorted.push_back({index, entry.content_start, entry.content_end});
  }
  if(sorted.empty()) {
    return result;
  }
  std::sort(sorted.begin(), sorted.end(), [](const SortedEntry& a, const SortedEntry& b) {
    return a.start < b.start || (a.start == b.start && a.index < b.index);
  });

  const float pin_line = scroll_offset + st.style.sticky_offset;
  const bool allow_exact = st.style.pin_at_rest || (has_scroll_controller && scroll_offset > kGeometryEpsilon);

  std::optional<std::size_t> candidate_pos;
  for(std::size_t k = 0; k < sorted.size(); ++k) {
    const SortedEntry& e = sorted[k];
    const bool passed_start =
        allow_exact ? (e.start <= pin_line + kGeometryEpsilon) : (e.start < pin_line - kGeometryEpsilon);
    const bool before_end = e.end > pin_line + kGeometryEpsilon;
    if(passed_start && before_end) {
      candidate_pos = k;
    }
  }
  if(!candidate_pos.has_value()) {
    return result;
  }

  const std::size_t k = *candidate_pos;
  float boundary = sorted[k].end;
  if(k + 1 < sorted.size()) {
    boundary = std::min(boundary, sorted[k + 1].start);
  }
  const float boundary_distance = std::max(0.0F, boundary - pin_line);
  float header_extent = 0.0F;
  if(const auto ext_it = st.header_extents.find(sorted[k].index); ext_it != st.header_extents.end()) {
    header_extent = ext_it->second;
  }
  const float progress = (header_extent > 0.0F && boundary_distance < header_extent)
                             ? std::clamp((header_extent - boundary_distance) / header_extent, 0.0F, 1.0F)
                             : 0.0F;

  result.pinned = true;
  result.active_index = sorted[k].index;
  result.boundary_distance = boundary_distance;
  result.push_progress = progress;
  return result;
}

bool ControllerOutOfSync(const detail::StickyHeaderControllerState& st, const StickyEvaluation& eval) {
  if(st.section_count.IsValid() && st.section_count.Get() != st.sections.size()) {
    return true;
  }
  const std::optional<std::size_t> next_pinned = eval.pinned ? std::optional{eval.active_index} : std::nullopt;
  if(st.pinned_section.IsValid() && st.pinned_section.Get() != next_pinned) {
    return true;
  }
  if(st.push_progress.IsValid() && std::abs(st.push_progress.Get() - eval.push_progress) > kProgressEpsilon) {
    return true;
  }
  if(!st.notified_initialized || st.last_notified_pinned != next_pinned) {
    return true;
  }
  return false;
}

void ScheduleSync(detail::StickyHeaderControllerState& state) {
  if(state.sync_scheduled || !state.alive || !*state.alive || !state.post_sync) return;
  state.sync_scheduled = true;
  state.post_sync();
}

struct StickyStageData {
  std::shared_ptr<detail::StickyHeaderControllerState> state;
  Axis axis = Axis::Vertical;
  float sticky_offset = 0.0F;
  bool pinned = false;
  std::size_t active_index = 0;
  float effective_boundary = std::numeric_limits<float>::infinity();

  bool operator==(const StickyStageData&) const = default;
};

struct StickyStageLayoutValue {
  using Value = StickyStageData;
};

class StickyHeaderStageLayout final : public Layout<StickyHeaderStageLayout> {
public:
  using Layout::Layout;

  static LayoutResult Measure(LayoutContext& context, ViewNode& node, const Constraints& constraints) {
    LayoutResult result;
    if(node.ChildCount() == 0U) {
      return result.SetSize(constraints.Constrain(Size{}));
    }
    const Size content_size = context.Measure(node.ChildAt(0U), constraints);
    result.Place(node.ChildAt(0U), Point{0.0F, 0.0F});

    const StickyStageData data = node.LayoutValueOr<StickyStageLayoutValue>(StickyStageData{});
    if(node.ChildCount() >= 2U && data.pinned) {
      const float w = std::max(0.0F, content_size.width);
      const float h = std::max(0.0F, content_size.height);
      const Constraints overlay_constraints =
          data.axis == Axis::Vertical ? Constraints{w, w, 0.0F, h} : Constraints{0.0F, w, h, h};
      const Size header_size = context.Measure(node.ChildAt(1U), overlay_constraints);
      const float header_extent = data.axis == Axis::Vertical ? header_size.height : header_size.width;
      if(data.state && header_extent > 0.0F) {
        data.state->header_extents[data.active_index] = header_extent;
      }
      const float push_offset =
          (std::isfinite(data.effective_boundary) && header_extent > 0.0F && data.effective_boundary < header_extent)
              ? (data.effective_boundary - header_extent)
              : 0.0F;
      const Point origin = data.axis == Axis::Vertical ? Point{0.0F, data.sticky_offset + push_offset}
                                                       : Point{data.sticky_offset + push_offset, 0.0F};
      result.Place(node.ChildAt(1U), origin);
    }
    return result.SetSize(constraints.Constrain(content_size));
  }
};

struct StickyHeaderViewportProbe {
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  std::shared_ptr<detail::StickyHeaderControllerState> state;

  bool operator==(const StickyHeaderViewportProbe&) const = default;
};

class StickyHeaderViewportProbe::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const StickyHeaderViewportProbe& value) { Update(node, value); }

  ~Extension() override {
    if(state_) {
      state_->start_smooth_scroll = {};
      state_->cancel_smooth_scroll = {};
    }
  }

  void Update(ViewNode&, const StickyHeaderViewportProbe& value) {
    const bool state_changed = state_ != value.state;
    if(state_ && state_changed) {
      state_->start_smooth_scroll = {};
      state_->cancel_smooth_scroll = {};
      animator_.Cancel();
    }
    state_ = value.state;
    if(state_) {
      state_->start_smooth_scroll = [this](float from, float to, const AnimationSpec& motion) {
        animator_.Start(from, to, motion);
        InvalidatePaint(PaintInvalidation::Foreground);
      };
      state_->cancel_smooth_scroll = [this]() { animator_.Cancel(); };
      if(state_changed) {
        state_->needs_sync = true;
      }
    }
  }

  FrameResult OnFrame(ViewNode&, const FrameInfo& frame) override {
    if(!animator_.IsActive()) return {};
    if(!state_ || !state_->style.scroll_controller.has_value() || !state_->style.scroll_controller->IsConnected()) {
      animator_.Cancel();
      return {};
    }
    return animator_.Advance(*state_->style.scroll_controller, frame);
  }

  PaintInvalidation PrepareGeometry(ViewNode& node, TextMeasurer&) override {
    if(!state_) return PaintInvalidation::None;
    const Rect bounds = node.PresentationBounds();
    const bool valid = !bounds.IsEmpty();
    const bool bounds_changed = state_->has_viewport != valid || state_->viewport_bounds != bounds;
    if(bounds_changed) {
      state_->has_viewport = valid;
      state_->viewport_bounds = bounds;
      state_->layout_dirty = true;
    }
    bool has_sc = false;
    const float scroll_offset = CurrentScrollOffset(*state_, &has_sc);
    const StickyEvaluation eval = EvaluateSticky(*state_, scroll_offset, has_sc);
    if(bounds_changed || state_->needs_sync || ControllerOutOfSync(*state_, eval)) {
      state_->needs_sync = false;
      ScheduleSync(*state_);
    }
    return PaintInvalidation::None;
  }

private:
  std::shared_ptr<detail::StickyHeaderControllerState> state_;
  internal::ScrollAnimator animator_;
};

const huxerui::detail::ModifierDescriptor& StickyHeaderViewportProbe::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<StickyHeaderViewportProbe, StickyHeaderViewportProbe::Extension>();
}

struct StickyHeaderSectionProbe {
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  std::shared_ptr<detail::StickyHeaderControllerState> state;
  std::size_t index = 0;
  std::uint64_t token = 0;

  bool operator==(const StickyHeaderSectionProbe&) const = default;
};

class StickyHeaderSectionProbe::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const StickyHeaderSectionProbe& value) { Update(node, value); }

  void Update(ViewNode&, const StickyHeaderSectionProbe& value) {
    state_ = value.state;
    index_ = value.index;
    token_ = value.token;
  }

  PaintInvalidation PrepareGeometry(ViewNode& node, TextMeasurer&) override {
    if(!state_ || !state_->has_viewport) return PaintInvalidation::None;
    const auto it = state_->sections.find(index_);
    if(it != state_->sections.end() && it->second.token == token_) {
      const Rect bounds = node.PresentationBounds();
      const bool valid = !bounds.IsEmpty();
      if(!valid) {
        if(it->second.has_bounds) {
          it->second.has_bounds = false;
          state_->layout_dirty = true;
          ScheduleSync(*state_);
        }
        return PaintInvalidation::None;
      }

      const float live_offset = CurrentScrollOffset(*state_);
      const bool vertical = state_->style.axis == Axis::Vertical;
      const float vp_start = vertical ? state_->viewport_bounds.y : state_->viewport_bounds.x;
      const float sec_start = vertical ? bounds.y : bounds.x;
      const float sec_extent = vertical ? bounds.height : bounds.width;
      const float content_start = (sec_start - vp_start) + live_offset;
      const float content_end = content_start + sec_extent;

      if(!it->second.has_bounds || std::abs(it->second.content_start - content_start) > kGeometryEpsilon
         || std::abs(it->second.content_end - content_end) > kGeometryEpsilon) {
        it->second.content_start = content_start;
        it->second.content_end = content_end;
        it->second.has_bounds = true;
        state_->layout_dirty = true;
        ScheduleSync(*state_);
      }
    }
    return PaintInvalidation::None;
  }

private:
  std::shared_ptr<detail::StickyHeaderControllerState> state_;
  std::size_t index_ = 0;
  std::uint64_t token_ = 0;
};

const huxerui::detail::ModifierDescriptor& StickyHeaderSectionProbe::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<StickyHeaderSectionProbe, StickyHeaderSectionProbe::Extension>();
}

} // namespace

std::optional<std::size_t> StickyHeaderController::PinnedSection() const {
  if(!state_ || !state_->pinned_section.IsValid()) return std::nullopt;
  return state_->pinned_section.Get();
}

bool StickyHeaderController::IsPinned() const { return PinnedSection().has_value(); }

float StickyHeaderController::PushProgress() const {
  if(!state_ || !state_->push_progress.IsValid()) return 0.0F;
  return state_->push_progress.Get();
}

std::size_t StickyHeaderController::SectionCount() const {
  if(!state_ || !state_->section_count.IsValid()) return 0U;
  return state_->section_count.Get();
}

bool StickyHeaderController::ScrollToSection(std::size_t index, std::optional<ScrollJumpBehavior> behavior) const {
  if(!state_ || !state_->has_viewport || !state_->style.scroll_controller.has_value()
     || !state_->style.scroll_controller->IsConnected()) {
    return false;
  }
  const auto it = state_->sections.find(index);
  if(it == state_->sections.end() || !it->second.has_bounds) {
    return false;
  }
  const ScrollController sc = *state_->style.scroll_controller;
  const ScrollMetrics metrics = sc.Metrics();
  const float current_offset = metrics.offset;
  const float raw_target = it->second.content_start - state_->style.sticky_offset;
  const float clamped_target = std::clamp(raw_target, 0.0F, std::max(0.0F, metrics.maximum_offset));
  const ScrollJumpBehavior resolved = behavior.value_or(state_->style.jump_behavior);
  if(resolved == ScrollJumpBehavior::Instant || !state_->start_smooth_scroll
     || std::abs(clamped_target - current_offset) <= 0.5F) {
    if(state_->cancel_smooth_scroll) {
      state_->cancel_smooth_scroll();
    }
    return sc.ScrollTo(clamped_target);
  }
  state_->start_smooth_scroll(current_offset, clamped_target, state_->style.scroll_motion);
  return true;
}

StickyHeaderController UseStickyHeaderController() {
  auto pinned_section = UseState<std::optional<std::size_t>>(std::nullopt);
  auto push_progress = UseState(0.0F);
  auto section_count = UseState<std::size_t>(0U);
  auto holder = UseState(std::make_shared<detail::StickyHeaderControllerState>());
  auto shared = holder.Get();
  shared->pinned_section = pinned_section;
  shared->push_progress = push_progress;
  shared->section_count = section_count;
  return StickyHeaderController(std::move(shared));
}

View StickyHeaderArea(View scrollable_content, std::function<View(StickyHeaderSnapshot)> pinned_header_builder,
                      StickyHeaderStyle style, std::function<void(std::optional<std::size_t>)> on_pinned_changed) {
  if(!pinned_header_builder) {
    throw std::invalid_argument("HuxerUI Interaction: StickyHeaderArea requires a pinned header builder");
  }
  ValidateStyle(style);

  return Scope([scrollable_content = std::move(scrollable_content),
                pinned_header_builder = std::move(pinned_header_builder), style = std::move(style),
                on_pinned_changed = std::move(on_pinned_changed)]() -> View {
    TaskScope tasks = UseTaskScope();
    auto alive = UseState(std::make_shared<bool>(true)).Get();
    auto local_pinned = UseState<std::optional<std::size_t>>(std::nullopt);
    auto local_progress = UseState(0.0F);
    auto local_count = UseState<std::size_t>(0U);
    auto geometry_version = UseState<std::uint64_t>(0U);
    auto local_holder = UseState(std::make_shared<detail::StickyHeaderControllerState>());

    std::shared_ptr<detail::StickyHeaderControllerState> state =
        style.controller.state_ ? style.controller.state_ : local_holder.Get();
    if(!style.controller.state_) {
      state->pinned_section = local_pinned;
      state->push_progress = local_progress;
      state->section_count = local_count;
    }
    state->geometry_version = geometry_version;
    state->style = style;
    state->style.controller = {};
    state->on_pinned_changed = on_pinned_changed;
    state->alive = alive;
    *alive = true;

    std::weak_ptr<detail::StickyHeaderControllerState> weak_state = state;
    state->post_sync = [tasks, weak_state, alive]() {
      tasks.Post([weak_state, alive]() {
        if(!*alive) return;
        auto locked = weak_state.lock();
        if(!locked) return;
        locked->sync_scheduled = false;

        if(locked->section_count.IsValid() && locked->section_count.Get() != locked->sections.size()) {
          locked->section_count = locked->sections.size();
        }
        bool has_sc = false;
        const float scroll_offset = CurrentScrollOffset(*locked, &has_sc);
        const StickyEvaluation eval = EvaluateSticky(*locked, scroll_offset, has_sc);
        const std::optional<std::size_t> next_pinned = eval.pinned ? std::optional{eval.active_index} : std::nullopt;

        if(locked->pinned_section.IsValid() && locked->pinned_section.Get() != next_pinned) {
          locked->pinned_section = next_pinned;
        }
        if(locked->push_progress.IsValid() && std::abs(locked->push_progress.Get() - eval.push_progress) > kProgressEpsilon) {
          locked->push_progress = eval.push_progress;
        }
        if(locked->layout_dirty) {
          locked->layout_dirty = false;
          if(locked->geometry_version.IsValid()) {
            locked->geometry_version = locked->geometry_version.Get() + 1U;
          }
        }
        if(!locked->notified_initialized || locked->last_notified_pinned != next_pinned) {
          locked->last_notified_pinned = next_pinned;
          locked->notified_initialized = true;
          if(locked->on_pinned_changed) {
            auto cb = locked->on_pinned_changed;
            cb(next_pinned);
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

    View tracked = ProvideEnvironment(StickyHeaderEnvironment{state}, scrollable_content);

    return Scope([tracked = std::move(tracked), pinned_header_builder, state, geometry_version]() -> View {
      (void)geometry_version.Get();
      bool has_sc = false;
      const float scroll_offset = CurrentScrollOffset(*state, &has_sc);
      const StickyEvaluation eval = EvaluateSticky(*state, scroll_offset, has_sc);

      float effective_boundary = std::numeric_limits<float>::infinity();
      if(eval.pinned) {
        const auto ext_it = state->header_extents.find(eval.active_index);
        if(ext_it == state->header_extents.end() || ext_it->second <= 0.0F || eval.boundary_distance < ext_it->second) {
          effective_boundary = eval.boundary_distance;
        }
      }

      const StickyStageData stage_data{
          .state = state,
          .axis = state->style.axis,
          .sticky_offset = state->style.sticky_offset,
          .pinned = eval.pinned,
          .active_index = eval.active_index,
          .effective_boundary = effective_boundary,
      };

      std::vector<View> stage_children;
      stage_children.reserve(eval.pinned ? 2U : 1U);
      stage_children.push_back(tracked);
      if(eval.pinned) {
        stage_children.push_back(pinned_header_builder(StickyHeaderSnapshot{
            .index = eval.active_index,
            .pinned = true,
            .push_progress = eval.push_progress,
        }));
      }

      return StickyHeaderStageLayout(std::move(stage_children))
          .LayoutValue<StickyStageLayoutValue>(stage_data)
          .With(ClipChildren{}, StickyHeaderViewportProbe{.state = state});
    });
  });
}

View StickyHeaderSection(View content, std::size_t index) {
  return Scope([content = std::move(content), index]() -> View {
    const auto& env = UseEnvironment<StickyHeaderEnvironment>();
    auto token_holder = UseState(std::make_shared<StickyHeaderTokenHolder>()).Get();
    View section = content;
    if(env.state) {
      if(token_holder->token == 0U) {
        token_holder->token = env.state->next_token++;
      }
      const std::uint64_t token = token_holder->token;
      const auto existing = env.state->sections.find(index);
      if(existing == env.state->sections.end() || existing->second.token != token) {
        env.state->sections.insert_or_assign(index, detail::StickyHeaderSectionEntry{.token = token});
      }
      std::weak_ptr<detail::StickyHeaderControllerState> weak_state = env.state;
      Lifecycle(
          [weak_state, index, token]() {
            return [weak_state, index, token]() {
              if(auto locked = weak_state.lock()) {
                const auto it = locked->sections.find(index);
                if(it != locked->sections.end() && it->second.token == token) {
                  locked->sections.erase(it);
                  locked->needs_sync = true;
                  locked->layout_dirty = true;
                }
              }
            };
          },
          env.state, index, token);
      section = std::move(section).With(StickyHeaderSectionProbe{
          .state = env.state,
          .index = index,
          .token = token,
      });
    }
    return section;
  });
}

View StickyHeaderList(std::vector<StickyHeaderGroup> groups, StickyHeaderStyle style,
                      std::function<void(std::optional<std::size_t>)> on_pinned_changed) {
  ValidateStyle(style);
  for(const auto& group : groups) {
    if(!group.header) {
      throw std::invalid_argument("HuxerUI Interaction: StickyHeaderList group header builder must not be empty");
    }
  }

  return Scope([groups = std::move(groups), style = std::move(style),
                on_pinned_changed = std::move(on_pinned_changed)]() -> View {
    ScrollController effective_scroll =
        style.scroll_controller.has_value() ? *style.scroll_controller : UseScrollController();
    StickyHeaderController effective_ctrl =
        style.controller ? style.controller : UseStickyHeaderController();

    StickyHeaderStyle effective_style = style;
    effective_style.scroll_controller = effective_scroll;
    effective_style.controller = effective_ctrl;

    std::vector<View> items;
    items.reserve(groups.size());
    for(std::size_t i = 0; i < groups.size(); ++i) {
      View in_flow_header = Scope([header = groups[i].header, effective_ctrl, i]() -> View {
        const bool is_pinned = effective_ctrl.PinnedSection() == std::optional<std::size_t>{i};
        return header(StickyHeaderSnapshot{
                          .index = i,
                          .pinned = false,
                          .push_progress = 0.0F,
                      })
            .With(Opacity(is_pinned ? 0.0F : 1.0F));
      });
      View group_view =
          effective_style.axis == Axis::Vertical
              ? View(Column{std::move(in_flow_header), groups[i].body}.With(CrossAlign(CrossAxisAlignment::Stretch)))
              : View(Row{std::move(in_flow_header), groups[i].body}.With(CrossAlign(CrossAxisAlignment::Stretch)));
      items.push_back(StickyHeaderSection(std::move(group_view), i));
    }

    View container =
        effective_style.axis == Axis::Vertical
            ? View(Column(std::move(items))
                       .With(Spacing(effective_style.section_spacing), CrossAlign(CrossAxisAlignment::Stretch)))
            : View(Row(std::move(items))
                       .With(Spacing(effective_style.section_spacing), CrossAlign(CrossAxisAlignment::Stretch)));

    View scrollable = ScrollView(std::move(container)).ScrollAxis(effective_style.axis).Controller(effective_scroll);

    auto pinned_builder = [groups](StickyHeaderSnapshot snapshot) -> View {
      if(snapshot.index < groups.size() && groups[snapshot.index].header) {
        return groups[snapshot.index].header(snapshot);
      }
      return Stack{};
    };

    return StickyHeaderArea(std::move(scrollable), std::move(pinned_builder), std::move(effective_style),
                            on_pinned_changed);
  });
}

} // namespace huxerui::interaction
