#include <huxerui/interaction/scroll/scrollreveal.h>

#include "../internal/validate.h"

#include <huxerui/environment.h>
#include <huxerui/layout.h>
#include <huxerui/lifecycle.h>
#include <huxerui/modifier.h>
#include <huxerui/state.h>
#include <huxerui/task.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

namespace huxerui::interaction {
namespace {

constexpr float kGeometryEpsilon = 0.25F;
constexpr float kRatioEpsilon = 0.01F;
constexpr float kThresholdEpsilon = 1e-4F;

void ValidateAreaStyle(const ScrollRevealAreaStyle& style) {
  internal::RequireFinite(style.viewport_margin,
                          "HuxerUI Interaction: ScrollRevealArea viewport margin must be finite");
}

void ValidateStyle(const ScrollRevealStyle& style) {
  internal::RequireUnitInterval(style.threshold,
                                "HuxerUI Interaction: ScrollReveal threshold must be finite and in [0, 1]");
  internal::RequireNonNegative(style.delay,
                               "HuxerUI Interaction: ScrollReveal delay must be finite and non-negative");
  internal::RequireUnitInterval(style.initial_opacity,
                                "HuxerUI Interaction: ScrollReveal initial opacity must be finite and in [0, 1]");
  internal::RequireFinite(style.initial_offset,
                          "HuxerUI Interaction: ScrollReveal initial offset must be finite");
  internal::RequirePositive(style.initial_scale,
                            "HuxerUI Interaction: ScrollReveal initial scale must be finite and positive");
  internal::RequireFinite(style.origin.x, "HuxerUI Interaction: ScrollReveal origin x must be finite");
  internal::RequireFinite(style.origin.y, "HuxerUI Interaction: ScrollReveal origin y must be finite");
  if(const auto* tween = std::get_if<TweenSpec>(&style.motion)) {
    internal::RequireNonNegative(tween->duration,
                                 "HuxerUI Interaction: ScrollReveal motion duration must be finite and non-negative");
  } else if(const auto* spring = std::get_if<SpringSpec>(&style.motion)) {
    internal::RequirePositive(spring->stiffness,
                              "HuxerUI Interaction: ScrollReveal motion spring stiffness must be finite and positive");
    internal::RequirePositive(
        spring->damping_ratio,
        "HuxerUI Interaction: ScrollReveal motion spring damping ratio must be finite and positive");
  }
}

class ScrollRevealPassThroughLayout final : public Layout<ScrollRevealPassThroughLayout> {
public:
  using Layout::Layout;

  static LayoutResult Measure(LayoutContext& context, ViewNode& node, const Constraints& constraints) {
    LayoutResult result;
    if(node.ChildCount() == 0U) {
      return result.SetSize(constraints.Constrain(Size{}));
    }
    const Size size = context.Measure(node.ChildAt(0U), constraints);
    result.Place(node.ChildAt(0U), Point{0.0F, 0.0F});
    return result.SetSize(constraints.Constrain(size));
  }
};

struct ScrollRevealAreaState {
  ScrollRevealAreaStyle style;
  Rect viewport_bounds{};
  bool has_viewport = false;
  State<std::uint64_t> viewport_version;
  std::shared_ptr<bool> alive;
  bool sync_scheduled = false;
  std::function<void()> post_sync;
};

struct ScrollRevealEnvironment {
  std::shared_ptr<ScrollRevealAreaState> area;

  static ScrollRevealEnvironment Default() { return {}; }

  bool operator==(const ScrollRevealEnvironment&) const = default;
};

float CurrentAreaScrollOffset(const ScrollRevealAreaState& area) {
  if(area.style.scroll_controller.has_value()) {
    const ScrollMetrics metrics = area.style.scroll_controller->Metrics();
    if(area.style.scroll_controller->IsConnected() && metrics.axis == area.style.axis) {
      return metrics.offset;
    }
  }
  return 0.0F;
}

void ScheduleAreaSync(ScrollRevealAreaState& area) {
  if(area.sync_scheduled || !area.alive || !*area.alive || !area.post_sync) return;
  area.sync_scheduled = true;
  area.post_sync();
}

struct ScrollRevealViewportProbe {
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  std::shared_ptr<ScrollRevealAreaState> area;

  bool operator==(const ScrollRevealViewportProbe&) const = default;
};

class ScrollRevealViewportProbe::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const ScrollRevealViewportProbe& value) { Update(node, value); }

  void Update(ViewNode&, const ScrollRevealViewportProbe& value) { area_ = value.area; }

  PaintInvalidation PrepareGeometry(ViewNode& node, TextMeasurer&) override {
    if(!area_) return PaintInvalidation::None;
    const Rect bounds = node.PresentationBounds();
    const bool valid = !bounds.IsEmpty();
    const bool changed = area_->has_viewport != valid
                         || std::abs(area_->viewport_bounds.x - bounds.x) > kGeometryEpsilon
                         || std::abs(area_->viewport_bounds.y - bounds.y) > kGeometryEpsilon
                         || std::abs(area_->viewport_bounds.width - bounds.width) > kGeometryEpsilon
                         || std::abs(area_->viewport_bounds.height - bounds.height) > kGeometryEpsilon;
    if(changed) {
      area_->viewport_bounds = bounds;
      area_->has_viewport = valid;
      ScheduleAreaSync(*area_);
    }
    return PaintInvalidation::None;
  }

private:
  std::shared_ptr<ScrollRevealAreaState> area_;
};

const huxerui::detail::ModifierDescriptor& ScrollRevealViewportProbe::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<ScrollRevealViewportProbe, ScrollRevealViewportProbe::Extension>();
}

struct RevealEvaluation {
  bool in_viewport = false;
  float ratio = 0.0F;
};

struct ScrollRevealItemState {
  std::shared_ptr<ScrollRevealAreaState> area;
  ScrollRevealStyle style;
  std::function<void(bool)> on_visibility_changed;
  TaskScope tasks;
  State<bool> visible_state;
  State<bool> in_viewport_state;
  State<float> ratio_state;

  float content_start = 0.0F;
  float content_extent = 0.0F;
  bool has_bounds = false;

  bool revealed_local = false;
  bool published_in_viewport = false;
  float published_ratio = 0.0F;
  bool delay_pending = false;
  std::uint64_t delay_gen = 0;
  TaskHandle delay_task;

  bool sync_scheduled = false;
  std::shared_ptr<bool> alive;
  std::function<void()> post_sync;
};

RevealEvaluation EvaluateReveal(const ScrollRevealItemState& item) {
  if(!item.area) {
    return {.in_viewport = true, .ratio = 1.0F};
  }
  if(!item.area->has_viewport || !item.has_bounds) {
    return {.in_viewport = false, .ratio = 0.0F};
  }
  const Rect& vp = item.area->viewport_bounds;
  const float vp_extent = item.area->style.axis == Axis::Vertical ? vp.height : vp.width;
  const float margin = item.area->style.viewport_margin;
  const float eff_min = -margin;
  const float eff_max = vp_extent + margin;
  if(eff_max <= eff_min) {
    return {.in_viewport = false, .ratio = 0.0F};
  }
  const float live_offset = CurrentAreaScrollOffset(*item.area);
  const float live_start = item.content_start - live_offset;
  const float live_end = live_start + item.content_extent;
  const float intersection = std::max(0.0F, std::min(live_end, eff_max) - std::max(live_start, eff_min));
  const float denom = std::max(1.0F, std::min(item.content_extent, eff_max - eff_min));
  const float ratio = std::clamp(intersection / denom, 0.0F, 1.0F);
  const bool in_vp =
      (item.style.threshold <= 0.0F) ? (intersection > 0.0F) : (ratio + kThresholdEpsilon >= item.style.threshold);
  return {.in_viewport = in_vp, .ratio = ratio};
}

void ScheduleItemSync(ScrollRevealItemState& item) {
  if(item.sync_scheduled || !item.alive || !*item.alive || !item.post_sync) return;
  item.sync_scheduled = true;
  item.post_sync();
}

struct ScrollRevealItemProbe {
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  std::shared_ptr<ScrollRevealItemState> item;

  bool operator==(const ScrollRevealItemProbe&) const = default;
};

class ScrollRevealItemProbe::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const ScrollRevealItemProbe& value) { Update(node, value); }

  void Update(ViewNode&, const ScrollRevealItemProbe& value) { item_ = value.item; }

  PaintInvalidation PrepareGeometry(ViewNode& node, TextMeasurer&) override {
    if(!item_) return PaintInvalidation::None;

    if(item_->area) {
      if(!item_->area->has_viewport) {
        return PaintInvalidation::None;
      }
      const Rect bounds = node.PresentationBounds();
      const bool valid = !bounds.IsEmpty();
      if(!valid) {
        if(item_->has_bounds) {
          item_->has_bounds = false;
        }
      } else {
        const bool vertical = item_->area->style.axis == Axis::Vertical;
        const float vp_start = vertical ? item_->area->viewport_bounds.y : item_->area->viewport_bounds.x;
        const float item_start = vertical ? bounds.y : bounds.x;
        const float item_extent = vertical ? bounds.height : bounds.width;
        const float live_offset = CurrentAreaScrollOffset(*item_->area);
        const float content_start = (item_start - vp_start) + live_offset;
        if(!item_->has_bounds || std::abs(item_->content_start - content_start) > kGeometryEpsilon
           || std::abs(item_->content_extent - item_extent) > kGeometryEpsilon) {
          item_->content_start = content_start;
          item_->content_extent = item_extent;
          item_->has_bounds = true;
        }
      }
    }

    const RevealEvaluation eval = EvaluateReveal(*item_);
    const bool in_vp_changed = eval.in_viewport != item_->published_in_viewport;
    const bool ratio_changed = std::abs(eval.ratio - item_->published_ratio) > kRatioEpsilon;
    const bool should_trigger_reveal = eval.in_viewport && !item_->revealed_local && !item_->delay_pending;
    const bool should_trigger_hide = !eval.in_viewport && !item_->style.once && item_->revealed_local;
    const bool should_cancel_delay = !eval.in_viewport && item_->delay_pending;

    if(in_vp_changed || ratio_changed || should_trigger_reveal || should_trigger_hide || should_cancel_delay) {
      ScheduleItemSync(*item_);
    }
    return PaintInvalidation::None;
  }

private:
  std::shared_ptr<ScrollRevealItemState> item_;
};

const huxerui::detail::ModifierDescriptor& ScrollRevealItemProbe::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<ScrollRevealItemProbe, ScrollRevealItemProbe::Extension>();
}

} // namespace

View ScrollRevealArea(View scrollable_content, ScrollRevealAreaStyle style) {
  ValidateAreaStyle(style);
  return Scope([scrollable_content = std::move(scrollable_content), style = std::move(style)]() -> View {
    TaskScope tasks = UseTaskScope();
    auto alive = UseState(std::make_shared<bool>(true)).Get();
    auto viewport_version = UseState<std::uint64_t>(0U);
    auto holder = UseState(std::make_shared<ScrollRevealAreaState>());
    std::shared_ptr<ScrollRevealAreaState> area = holder.Get();

    area->style = style;
    area->viewport_version = viewport_version;
    area->alive = alive;
    *alive = true;

    std::weak_ptr<ScrollRevealAreaState> weak_area = area;
    area->post_sync = [tasks, weak_area, alive]() {
      tasks.Post([weak_area, alive]() {
        if(!*alive) return;
        auto locked = weak_area.lock();
        if(!locked) return;
        locked->sync_scheduled = false;
        if(locked->viewport_version.IsValid()) {
          locked->viewport_version = locked->viewport_version.Get() + 1U;
        }
      });
    };

    Lifecycle(
        [weak_area, alive]() {
          *alive = true;
          return [weak_area, alive]() {
            *alive = false;
            if(auto locked = weak_area.lock()) {
              locked->post_sync = {};
              locked->sync_scheduled = false;
            }
          };
        },
        area);

    std::vector<View> children;
    children.push_back(ProvideEnvironment(ScrollRevealEnvironment{area}, scrollable_content));
    return ScrollRevealPassThroughLayout(std::move(children)).With(ScrollRevealViewportProbe{.area = area});
  });
}

View ScrollReveal(std::function<View(ScrollRevealSnapshot)> builder, ScrollRevealStyle style,
                  std::function<void(bool visible)> on_visibility_changed) {
  if(!builder) {
    throw std::invalid_argument("HuxerUI Interaction: ScrollReveal requires a view builder");
  }
  ValidateStyle(style);

  return Scope([builder = std::move(builder), style = std::move(style),
                on_visibility_changed = std::move(on_visibility_changed)]() -> View {
    TaskScope tasks = UseTaskScope();
    const auto& env = UseEnvironment<ScrollRevealEnvironment>();
    auto alive = UseState(std::make_shared<bool>(true)).Get();
    auto visible_state = UseState(false);
    auto in_viewport_state = UseState(false);
    auto ratio_state = UseState(0.0F);
    auto holder = UseState(std::make_shared<ScrollRevealItemState>());
    std::shared_ptr<ScrollRevealItemState> item = holder.Get();

    item->area = env.area;
    item->style = style;
    item->on_visibility_changed = on_visibility_changed;
    item->tasks = tasks;
    item->visible_state = visible_state;
    item->in_viewport_state = in_viewport_state;
    item->ratio_state = ratio_state;
    item->alive = alive;
    *alive = true;

    std::weak_ptr<ScrollRevealItemState> weak_item = item;
    item->post_sync = [tasks, weak_item, alive]() {
      tasks.Post([weak_item, alive]() {
        if(!*alive) return;
        std::shared_ptr<ScrollRevealItemState> locked = weak_item.lock();
        if(!locked) return;
        locked->sync_scheduled = false;

        const RevealEvaluation eval = EvaluateReveal(*locked);
        locked->published_in_viewport = eval.in_viewport;
        locked->published_ratio = eval.ratio;

        if(locked->in_viewport_state.IsValid() && locked->in_viewport_state.Get() != eval.in_viewport) {
          locked->in_viewport_state = eval.in_viewport;
        }
        if(locked->ratio_state.IsValid() && std::abs(locked->ratio_state.Get() - eval.ratio) > kRatioEpsilon) {
          locked->ratio_state = eval.ratio;
        }

        if(eval.in_viewport) {
          if(!locked->revealed_local) {
            if(locked->style.delay <= 0.0) {
              locked->revealed_local = true;
              if(locked->visible_state.IsValid()) {
                locked->visible_state = true;
              }
              if(locked->on_visibility_changed) {
                auto cb = locked->on_visibility_changed;
                cb(true);
              }
            } else if(!locked->delay_pending) {
              locked->delay_pending = true;
              const std::uint64_t gen = ++locked->delay_gen;
              const double delay_seconds = locked->style.delay;
              locked->delay_task = locked->tasks.Launch([weak_item, alive, gen, delay_seconds]() -> Task<void> {
                co_await Delay(std::chrono::duration<double>(delay_seconds));
                std::shared_ptr<ScrollRevealItemState> self = weak_item.lock();
                if(!self || !*alive || self->delay_gen != gen || !self->published_in_viewport) {
                  co_return;
                }
                self->delay_pending = false;
                if(!self->revealed_local) {
                  self->revealed_local = true;
                  if(self->visible_state.IsValid()) {
                    self->visible_state = true;
                  }
                  if(self->on_visibility_changed) {
                    auto cb = self->on_visibility_changed;
                    cb(true);
                  }
                }
              });
            }
          }
        } else {
          if(locked->delay_pending) {
            locked->delay_task.Cancel();
            locked->delay_task = {};
            locked->delay_pending = false;
            ++locked->delay_gen;
          }
          if(!locked->style.once && locked->revealed_local) {
            locked->revealed_local = false;
            if(locked->visible_state.IsValid()) {
              locked->visible_state = false;
            }
            if(locked->on_visibility_changed) {
              auto cb = locked->on_visibility_changed;
              cb(false);
            }
          }
        }
      });
    };

    Lifecycle(
        [weak_item, alive]() {
          *alive = true;
          return [weak_item, alive]() {
            *alive = false;
            if(auto locked = weak_item.lock()) {
              locked->delay_task.Cancel();
              locked->delay_task = {};
              ++locked->delay_gen;
              locked->delay_pending = false;
              locked->post_sync = {};
              locked->sync_scheduled = false;
            }
          };
        },
        item);

    View inner = Scope([builder, item]() -> View {
      if(item->area) {
        (void)item->area->viewport_version.Get();
        (void)CurrentAreaScrollOffset(*item->area);
      }
      const ScrollRevealSnapshot snapshot{
          .visible = item->visible_state.Get(),
          .in_viewport = item->in_viewport_state.Get(),
          .visibility_ratio = item->ratio_state.Get(),
      };
      return builder(snapshot);
    });

    std::vector<View> children;
    children.push_back(std::move(inner));
    return ScrollRevealPassThroughLayout(std::move(children)).With(ScrollRevealItemProbe{.item = item});
  });
}

View ScrollReveal(View content, ScrollRevealStyle style,
                  std::function<void(bool visible)> on_visibility_changed) {
  ValidateStyle(style);
  return ScrollReveal(
      [content = std::move(content), style](ScrollRevealSnapshot s) -> View {
        View inner = content;
        return std::move(inner)
            .With(Opacity(AnimateTo(s.visible ? 1.0F : style.initial_opacity, style.motion)))
            .With(Offset(AnimateTo(s.visible ? Point{} : style.initial_offset, style.motion)))
            .With(Scale(AnimateTo(s.visible ? 1.0F : style.initial_scale, style.motion), style.origin));
      },
      std::move(style), std::move(on_visibility_changed));
}

} // namespace huxerui::interaction
