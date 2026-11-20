#include <huxerui/interaction/scroll/parallax.h>

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
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace huxerui::interaction {
namespace {

constexpr float kGeometryEpsilon = 0.25F;

void ValidateStyle(const ScrollParallaxStyle& style) {
  internal::RequireFinite(style.speed, "HuxerUI Interaction: ScrollParallax speed must be finite");
  internal::RequireNonNegative(style.maximum_offset,
                               "HuxerUI Interaction: ScrollParallax maximum offset must be finite and non-negative");
  internal::RequirePositive(style.center_scale,
                            "HuxerUI Interaction: ScrollParallax center scale must be finite and positive");
  internal::RequireUnitInterval(style.edge_opacity,
                                "HuxerUI Interaction: ScrollParallax edge opacity must be finite and in [0, 1]");
  internal::RequireFinite(style.origin.x, "HuxerUI Interaction: ScrollParallax origin x must be finite");
  internal::RequireFinite(style.origin.y, "HuxerUI Interaction: ScrollParallax origin y must be finite");
}

class ScrollParallaxPassThroughLayout final : public Layout<ScrollParallaxPassThroughLayout> {
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

struct ScrollParallaxAreaState {
  ScrollParallaxAreaStyle style;
  Rect viewport_bounds{};
  bool has_viewport = false;
  State<std::uint64_t> viewport_version;
  std::shared_ptr<bool> alive;
  bool sync_scheduled = false;
  std::function<void()> post_sync;
};

struct ScrollParallaxEnvironment {
  std::shared_ptr<ScrollParallaxAreaState> area;

  static ScrollParallaxEnvironment Default() { return {}; }

  bool operator==(const ScrollParallaxEnvironment&) const = default;
};

float CurrentAreaScrollOffset(const ScrollParallaxAreaState& area) {
  if(area.style.scroll_controller.has_value()) {
    const ScrollMetrics metrics = area.style.scroll_controller->Metrics();
    if(area.style.scroll_controller->IsConnected() && metrics.axis == area.style.axis) {
      return metrics.offset;
    }
  }
  return 0.0F;
}

void ScheduleAreaSync(ScrollParallaxAreaState& area) {
  if(area.sync_scheduled || !area.alive || !*area.alive || !area.post_sync) return;
  area.sync_scheduled = true;
  area.post_sync();
}

struct ScrollParallaxViewportProbe {
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  std::shared_ptr<ScrollParallaxAreaState> area;

  bool operator==(const ScrollParallaxViewportProbe&) const = default;
};

class ScrollParallaxViewportProbe::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const ScrollParallaxViewportProbe& value) { Update(node, value); }

  void Update(ViewNode&, const ScrollParallaxViewportProbe& value) { area_ = value.area; }

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
  std::shared_ptr<ScrollParallaxAreaState> area_;
};

const huxerui::detail::ModifierDescriptor& ScrollParallaxViewportProbe::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<ScrollParallaxViewportProbe, ScrollParallaxViewportProbe::Extension>();
}

struct ScrollParallaxItemState {
  std::shared_ptr<ScrollParallaxAreaState> area;
  State<std::uint64_t> item_version;
  State<bool> reduced_motion;
  float content_start = 0.0F;
  float content_extent = 0.0F;
  bool has_bounds = false;
  bool last_frame_reduced = false;
  bool needs_geometry_bump = false;
  bool sync_scheduled = false;
  std::shared_ptr<bool> alive;
  std::function<void()> post_sync;
};

void ScheduleItemSync(ScrollParallaxItemState& item) {
  if(item.sync_scheduled || !item.alive || !*item.alive || !item.post_sync) return;
  item.sync_scheduled = true;
  item.post_sync();
}

struct ScrollParallaxItemProbe {
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  std::shared_ptr<ScrollParallaxItemState> item;

  bool operator==(const ScrollParallaxItemProbe&) const = default;
};

class ScrollParallaxItemProbe::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const ScrollParallaxItemProbe& value) { Update(node, value); }

  void Update(ViewNode&, const ScrollParallaxItemProbe& value) { item_ = value.item; }

  FrameResult OnFrame(ViewNode&, const FrameInfo& frame) override {
    if(!item_) return {};
    if(item_->last_frame_reduced != frame.reduced_motion) {
      item_->last_frame_reduced = frame.reduced_motion;
      ScheduleItemSync(*item_);
    }
    return {};
  }

  PaintInvalidation PrepareGeometry(ViewNode& node, TextMeasurer&) override {
    if(!item_ || !item_->area || !item_->area->has_viewport) {
      return PaintInvalidation::None;
    }
    const Rect bounds = node.PresentationBounds();
    const bool valid = !bounds.IsEmpty();
    if(!valid) {
      if(item_->has_bounds) {
        item_->has_bounds = false;
        item_->needs_geometry_bump = true;
        ScheduleItemSync(*item_);
      }
      return PaintInvalidation::None;
    }

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
      item_->needs_geometry_bump = true;
      ScheduleItemSync(*item_);
    }
    return PaintInvalidation::None;
  }

private:
  std::shared_ptr<ScrollParallaxItemState> item_;
};

const huxerui::detail::ModifierDescriptor& ScrollParallaxItemProbe::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<ScrollParallaxItemProbe, ScrollParallaxItemProbe::Extension>();
}

} // namespace

View ScrollParallaxArea(View scrollable_content, ScrollParallaxAreaStyle style) {
  return Scope([scrollable_content = std::move(scrollable_content), style = std::move(style)]() -> View {
    TaskScope tasks = UseTaskScope();
    auto alive = UseState(std::make_shared<bool>(true)).Get();
    auto viewport_version = UseState<std::uint64_t>(0U);
    auto holder = UseState(std::make_shared<ScrollParallaxAreaState>());
    std::shared_ptr<ScrollParallaxAreaState> area = holder.Get();

    area->style = style;
    area->viewport_version = viewport_version;
    area->alive = alive;
    *alive = true;

    std::weak_ptr<ScrollParallaxAreaState> weak_area = area;
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
    children.push_back(ProvideEnvironment(ScrollParallaxEnvironment{area}, scrollable_content));
    return ScrollParallaxPassThroughLayout(std::move(children)).With(ScrollParallaxViewportProbe{.area = area});
  });
}

View ScrollParallax(std::function<View(ScrollParallaxSnapshot)> builder, ScrollParallaxStyle style) {
  if(!builder) {
    throw std::invalid_argument("HuxerUI Interaction: ScrollParallax requires a view builder");
  }
  ValidateStyle(style);

  return Scope([builder = std::move(builder), style = std::move(style)]() -> View {
    TaskScope tasks = UseTaskScope();
    const auto& env = UseEnvironment<ScrollParallaxEnvironment>();
    auto alive = UseState(std::make_shared<bool>(true)).Get();
    auto item_version = UseState<std::uint64_t>(0U);
    auto reduced_motion = UseState(false);
    auto holder = UseState(std::make_shared<ScrollParallaxItemState>());
    std::shared_ptr<ScrollParallaxItemState> item = holder.Get();

    item->area = env.area;
    item->item_version = item_version;
    item->reduced_motion = reduced_motion;
    item->alive = alive;
    *alive = true;

    std::weak_ptr<ScrollParallaxItemState> weak_item = item;
    item->post_sync = [tasks, weak_item, alive]() {
      tasks.Post([weak_item, alive]() {
        if(!*alive) return;
        auto locked = weak_item.lock();
        if(!locked) return;
        locked->sync_scheduled = false;
        if(locked->reduced_motion.IsValid() && locked->reduced_motion.Get() != locked->last_frame_reduced) {
          locked->reduced_motion = locked->last_frame_reduced;
        }
        if(locked->needs_geometry_bump) {
          locked->needs_geometry_bump = false;
          if(locked->item_version.IsValid()) {
            locked->item_version = locked->item_version.Get() + 1U;
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
              locked->post_sync = {};
              locked->sync_scheduled = false;
            }
          };
        },
        item);

    View content = Scope([builder, style, item]() -> View {
      (void)item->item_version.Get();
      const bool is_reduced = item->reduced_motion.Get();

      Axis axis = style.axis;
      float progress = 0.0F;
      float visibility = 1.0F;
      float axis_shift = 0.0F;

      if(item->area) {
        (void)item->area->viewport_version.Get();
        axis = item->area->style.axis;
        const float live_offset = CurrentAreaScrollOffset(*item->area);
        if(item->area->has_viewport && item->has_bounds) {
          const Rect& vp = item->area->viewport_bounds;
          const float vp_extent = axis == Axis::Vertical ? vp.height : vp.width;
          const float live_start = item->content_start - live_offset;
          const float live_end = live_start + item->content_extent;
          const float live_center = live_start + item->content_extent * 0.5F;
          const float half_travel = std::max(1.0F, (vp_extent + item->content_extent) * 0.5F);
          progress = std::clamp((live_center - vp_extent * 0.5F) / half_travel, -1.0F, 1.0F);
          const float intersection = std::max(0.0F, std::min(live_end, vp_extent) - std::max(live_start, 0.0F));
          const float denom = std::max(1.0F, std::min(item->content_extent, vp_extent));
          visibility = std::clamp(intersection / denom, 0.0F, 1.0F);
          axis_shift =
              std::clamp(progress * style.maximum_offset * style.speed, -style.maximum_offset, style.maximum_offset);
        }
      } else if(style.scroll_controller.has_value()) {
        const ScrollMetrics metrics = style.scroll_controller->Metrics();
        if(style.scroll_controller->IsConnected() && metrics.axis == axis) {
          axis_shift = std::clamp(metrics.offset * style.speed, -style.maximum_offset, style.maximum_offset);
          progress =
              style.maximum_offset > 0.0F ? std::clamp(axis_shift / style.maximum_offset, -1.0F, 1.0F) : 0.0F;
          visibility = 1.0F;
        }
      }

      const bool suppress = style.respect_reduced_motion && is_reduced;
      const float center_weight = 1.0F - std::abs(progress);
      const ScrollParallaxSnapshot snapshot{
          .progress = progress,
          .visibility = visibility,
          .offset = suppress ? Point{}
                             : (axis == Axis::Vertical ? Point{0.0F, axis_shift} : Point{axis_shift, 0.0F}),
          .scale = suppress ? 1.0F : (1.0F + center_weight * (style.center_scale - 1.0F)),
          .opacity = suppress ? 1.0F : (style.edge_opacity + center_weight * (1.0F - style.edge_opacity)),
          .reduced_motion = is_reduced,
      };
      return builder(snapshot);
    });

    std::vector<View> children;
    children.push_back(std::move(content));
    View outer = ScrollParallaxPassThroughLayout(std::move(children)).With(ScrollParallaxItemProbe{.item = item});
    if(style.clip_to_bounds) {
      outer = std::move(outer).With(ClipChildren{});
    }
    return outer;
  });
}

View ScrollParallax(View content, ScrollParallaxStyle style) {
  ValidateStyle(style);
  const TransformOrigin origin = style.origin;
  return ScrollParallax(
      [content = std::move(content), origin](ScrollParallaxSnapshot s) -> View {
        View inner = content;
        return std::move(inner).With(Offset(s.offset), Scale(s.scale, origin), Opacity(s.opacity));
      },
      std::move(style));
}

} // namespace huxerui::interaction
