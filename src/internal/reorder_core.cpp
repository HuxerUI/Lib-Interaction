#include "reorder_core.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace huxerui::interaction::internal {
namespace {

void MaybeAutoScroll(const ReorderContext& context, Point window_position) {
  if(!context.auto_scroll_edge.has_value() || !context.scroll_controller.IsConnected() || !context.root_bounds) {
    return;
  }
  const Rect root = *context.root_bounds;
  const ScrollMetrics metrics = context.scroll_controller.Metrics();
  const float edge = std::min(*context.auto_scroll_edge,
                              (metrics.axis == Axis::Vertical ? root.height : root.width) * 0.5F);
  if(metrics.axis == Axis::Vertical) {
    if(window_position.y < root.y + edge) {
      (void)context.scroll_controller.ScrollBy(-context.auto_scroll_step);
    } else if(window_position.y > root.y + root.height - edge) {
      (void)context.scroll_controller.ScrollBy(context.auto_scroll_step);
    }
  } else {
    if(window_position.x < root.x + edge) {
      (void)context.scroll_controller.ScrollBy(-context.auto_scroll_step);
    } else if(window_position.x > root.x + root.width - edge) {
      (void)context.scroll_controller.ScrollBy(context.auto_scroll_step);
    }
  }
}

} // namespace

void SlotGeometry::Record(std::size_t index, Rect window_bounds) {
  slots_.insert_or_assign(index, window_bounds);
}

std::optional<Rect> SlotGeometry::Find(std::size_t index) const {
  const auto slot = slots_.find(index);
  if(slot == slots_.end()) return std::nullopt;
  return slot->second;
}

std::optional<std::size_t> SlotGeometry::Closest(std::size_t count, Point window_position) const {
  std::optional<std::size_t> closest;
  float closest_distance = std::numeric_limits<float>::infinity();
  for(std::size_t index = 0; index < count; ++index) {
    const auto slot = slots_.find(index);
    if(slot == slots_.end()) continue;
    const Rect& bounds = slot->second;
    const float dx = window_position.x - std::clamp(window_position.x, bounds.x, bounds.x + bounds.width);
    const float dy = window_position.y - std::clamp(window_position.y, bounds.y, bounds.y + bounds.height);
    const float distance = dx * dx + dy * dy;
    if(distance < closest_distance) {
      closest_distance = distance;
      closest = index;
    }
  }
  return closest;
}

class SlotBounds::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const SlotBounds& spec) { Update(node, spec); }

  void Update(ViewNode&, const SlotBounds& spec) {
    geometry_ = spec.geometry;
    index_ = spec.index;
  }

  [[nodiscard]] PaintInvalidation PrepareGeometry(ViewNode& node, TextMeasurer&) override {
    if(geometry_) {
      geometry_->Record(index_, node.LocalToWindowBounds(node.Bounds()));
    }
    return PaintInvalidation::None;
  }

private:
  std::shared_ptr<SlotGeometry> geometry_;
  std::size_t index_ = 0;
};

const huxerui::detail::ModifierDescriptor& SlotBounds::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<SlotBounds, SlotBounds::Extension>();
}

class RootBounds::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const RootBounds& spec) { Update(node, spec); }

  void Update(ViewNode&, const RootBounds& spec) { bounds_ = spec.bounds; }

  [[nodiscard]] PaintInvalidation PrepareGeometry(ViewNode& node, TextMeasurer&) override {
    if(bounds_) {
      *bounds_ = node.LocalToWindowBounds(node.Bounds());
    }
    return PaintInvalidation::None;
  }

private:
  std::shared_ptr<Rect> bounds_;
};

const huxerui::detail::ModifierDescriptor& RootBounds::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<RootBounds, RootBounds::Extension>();
}

std::size_t DisplacedIndex(std::size_t index, std::size_t from, std::size_t to) noexcept {
  if(from < to && index > from && index <= to) return index - 1;
  if(from > to && index >= to && index < from) return index + 1;
  return index;
}

Point ComputeSlotOffset(const ReorderSession& session, const SlotGeometry& geometry, std::size_t index) {
  if(!session.active) return {};
  if(index == session.from) return session.translation;
  const std::size_t displaced = DisplacedIndex(index, session.from, session.target);
  const auto own = geometry.Find(index);
  const auto target = geometry.Find(displaced);
  if(own.has_value() && target.has_value()) {
    return {target->x - own->x, target->y - own->y};
  }
  return {};
}

View BindReorderDrag(View view, const std::shared_ptr<ReorderContext>& context, std::size_t index,
                     DragGesture gesture) {
  if(!context || !context->drag.IsValid()) return view;
  return std::move(view)
      .With(gesture)
      .On<DragEvents::Started>([context, index](const DragEvent&) {
        if(context->drag.Get().active) return;
        context->drag = ReorderSession{.active = true, .from = index, .target = index, .translation = {}};
      })
      .On<DragEvents::Changed>([context, index](const DragEvent& event) {
        ReorderSession next = context->drag.Get();
        if(!next.active || next.from != index) return;
        next.translation = event.translation;
        if(context->geometry) {
          if(const auto closest = context->geometry->Closest(context->count, event.window_position)) {
            next.target = *closest;
          }
        }
        MaybeAutoScroll(*context, event.window_position);
        context->drag = next;
      })
      .On<DragEvents::Ended>([context, index](const DragEvent&) {
        const ReorderSession finished = context->drag.Get();
        if(!finished.active || finished.from != index) return;
        const auto callback = context->on_reordered;
        context->drag = ReorderSession{};
        if(callback && finished.target != index) {
          callback(finished.from, finished.target);
        }
      })
      .On<DragEvents::Canceled>([context, index](const DragEvent&) {
        const ReorderSession finished = context->drag.Get();
        if(finished.active && finished.from == index) {
          context->drag = ReorderSession{};
        }
      });
}

View BindKeyboardReorder(View view, const std::shared_ptr<ReorderContext>& context, std::size_t index, Axis axis,
                         bool two_dimensional) {
  if(!context) return view;
  return std::move(view)
      .With(Focusable{true})
      .On<ViewEvents::KeyIntercept>([context, index, axis, two_dimensional](const KeyEvent& event) {
        if(event.type != KeyEventType::Down || event.repeat || !event.modifiers.alt) return false;
        std::optional<int> delta;
        if(two_dimensional) {
          if(event.key == Key::ArrowLeft || event.key == Key::ArrowUp) delta = -1;
          if(event.key == Key::ArrowRight || event.key == Key::ArrowDown) delta = 1;
        } else if(axis == Axis::Vertical) {
          if(event.key == Key::ArrowUp) delta = -1;
          if(event.key == Key::ArrowDown) delta = 1;
        } else {
          if(event.key == Key::ArrowLeft) delta = -1;
          if(event.key == Key::ArrowRight) delta = 1;
        }
        if(!delta.has_value()) return false;
        const long target = static_cast<long>(index) + *delta;
        if(target < 0 || target >= static_cast<long>(context->count)) return false;
        if(context->on_reordered) {
          context->on_reordered(index, static_cast<std::size_t>(target));
        }
        return true;
      });
}

} // namespace huxerui::interaction::internal
