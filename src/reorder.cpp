#include <huxerui/interaction/reorder.h>

#include <huxerui/gesture.h>
#include <huxerui/modifier.h>
#include <huxerui/state.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const ReorderableStyle& style) {
  if(!std::isfinite(style.spacing) || style.spacing < 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: Reorderable spacing must be finite and non-negative");
  }
  if(!std::isfinite(style.target_highlight_width) || style.target_highlight_width < 0.0F) {
    throw std::invalid_argument(
        "HuxerUI Interaction: Reorderable highlight width must be finite and non-negative");
  }
  if(style.auto_scroll_edge.has_value()
     && (!std::isfinite(*style.auto_scroll_edge) || *style.auto_scroll_edge <= 0.0F)) {
    throw std::invalid_argument("HuxerUI Interaction: Reorderable auto-scroll edge must be finite and positive");
  }
  if(!std::isfinite(style.auto_scroll_step) || style.auto_scroll_step <= 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: Reorderable auto-scroll step must be finite and positive");
  }
}

/// Shared slot geometry recorded during layout preparation, keyed by slot index. Slots hold undragged layout
/// positions in window coordinates, so hit testing stays stable while displaced items animate.
class SlotGeometry {
public:
  void Record(std::size_t index, Rect window_bounds) { slots_.insert_or_assign(index, window_bounds); }

  [[nodiscard]] std::optional<Rect> Find(std::size_t index) const {
    const auto slot = slots_.find(index);
    if(slot == slots_.end()) return std::nullopt;
    return slot->second;
  }

  /// Returns the closest slot to a window position by point-to-rect distance, including the slot gap.
  [[nodiscard]] std::optional<std::size_t> Closest(std::size_t count, Point window_position) const {
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

private:
  std::unordered_map<std::size_t, Rect> slots_;
};

/// Retained extension recording each slot's undragged layout position on every geometry pass.
struct ReorderSlotBounds {
  /// Retained extension created by the framework when the modifier is attached through With().
  class Extension;

  static const detail::ModifierDescriptor& Descriptor();

  std::shared_ptr<SlotGeometry> geometry;
  std::size_t index = 0;

  bool operator==(const ReorderSlotBounds&) const = default;
};

class ReorderSlotBounds::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const ReorderSlotBounds& spec) { Update(node, spec); }

  void Update(ViewNode&, const ReorderSlotBounds& spec) {
    geometry_ = spec.geometry;
    index_ = spec.index;
  }

  [[nodiscard]] PaintInvalidation PrepareGeometry(ViewNode& node, TextMeasurer&) override {
    geometry_->Record(index_, node.LocalToWindowBounds(node.Bounds()));
    return PaintInvalidation::None;
  }

private:
  std::shared_ptr<SlotGeometry> geometry_;
  std::size_t index_ = 0;
};

const detail::ModifierDescriptor& ReorderSlotBounds::Descriptor() {
  return detail::ModifierDescriptorFor<ReorderSlotBounds, ReorderSlotBounds::Extension>();
}

/// Records the root list bounds in window coordinates for per-drag-update edge auto-scroll.
struct ReorderRootBounds {
  class Extension;
  static const detail::ModifierDescriptor& Descriptor();
  std::shared_ptr<Rect> bounds;
  bool operator==(const ReorderRootBounds&) const = default;
};

class ReorderRootBounds::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const ReorderRootBounds& spec) { Update(node, spec); }

  void Update(ViewNode&, const ReorderRootBounds& spec) { bounds_ = spec.bounds; }

  [[nodiscard]] PaintInvalidation PrepareGeometry(ViewNode& node, TextMeasurer&) override {
    if(bounds_) *bounds_ = node.LocalToWindowBounds(node.Bounds());
    return PaintInvalidation::None;
  }

private:
  std::shared_ptr<Rect> bounds_;
};

const detail::ModifierDescriptor& ReorderRootBounds::Descriptor() {
  return detail::ModifierDescriptorFor<ReorderRootBounds, ReorderRootBounds::Extension>();
}

/// One-slot displacement while a drag previews a destination: the dragged item holds its slot and the items
/// between the source and the target shift aside.
std::size_t DisplacedIndex(std::size_t index, std::size_t from, std::size_t to) {
  if(from < to && index > from && index <= to) return index - 1;
  if(from > to && index >= to && index < from) return index + 1;
  return index;
}

/// One mounted drag session over the whole widget; written as one value so every slot recomposes together.
struct DragSession {
  bool active = false;
  std::size_t from = 0;
  std::size_t target = 0;
  Point translation;

  bool operator==(const DragSession&) const = default;
};

} // namespace

View Reorderable(std::vector<View> items, ReorderableStyle style,
                 std::function<void(std::size_t from, std::size_t to)> on_reordered) {
  ValidateStyle(style);
  return Scope([items = std::move(items), style, on_reordered = std::move(on_reordered)]() -> View {
    auto geometry = UseState(std::make_shared<SlotGeometry>());
    auto root_bounds = UseState(std::make_shared<Rect>());
    auto drag = UseState(DragSession{});
    const DragSession session = drag.Get();
    const std::size_t count = items.size();
    const auto shared_geometry = geometry.Get();
    const auto shared_root_bounds = root_bounds.Get();
    std::vector<View> slots;
    slots.reserve(count);
    for(std::size_t index = 0; index < count; ++index) {
      Point offset{};
      const AnimationSpec* motion = &style.settle_motion;
      if(session.active && index == session.from) {
        offset = session.translation;
        motion = &style.follow_motion;
      } else if(session.active) {
        const std::size_t displaced = DisplacedIndex(index, session.from, session.target);
        const auto own = shared_geometry->Find(index);
        const auto target = shared_geometry->Find(displaced);
        if(own.has_value() && target.has_value()) {
          offset = {target->x - own->x, target->y - own->y};
        }
      }
      View item = std::move(items[index]);
      item = std::move(item).With(Offset(AnimateTo(offset, *motion)));
      View slot = Column{std::move(item)}
                      .With(ReorderSlotBounds{shared_geometry, index})
                      .With(DragGesture{})
                      .On<DragEvents::Started>(
                          [drag, index](const DragEvent&) {
                            if(drag.Get().active) return;
                            drag = DragSession{.active = true, .from = index, .target = index};
                          })
                      .On<DragEvents::Changed>(
                          [drag, index, shared_geometry, shared_root_bounds, count, style](const DragEvent& event) {
                            DragSession next = drag.Get();
                            if(!next.active || next.from != index) return;
                            next.translation = event.translation;
                            if(const auto closest = shared_geometry->Closest(count, event.window_position)) {
                              next.target = *closest;
                            }
                            if(style.auto_scroll_edge.has_value() && style.scroll_controller.IsConnected()
                               && shared_root_bounds) {
                              const Rect root = *shared_root_bounds;
                              const ScrollMetrics metrics = style.scroll_controller.Metrics();
                              const float edge = std::min(*style.auto_scroll_edge,
                                                          (metrics.axis == Axis::Vertical ? root.height : root.width)
                                                              * 0.5F);
                              if(metrics.axis == Axis::Vertical) {
                                if(event.window_position.y < root.y + edge) {
                                  (void)style.scroll_controller.ScrollBy(-style.auto_scroll_step);
                                } else if(event.window_position.y > root.y + root.height - edge) {
                                  (void)style.scroll_controller.ScrollBy(style.auto_scroll_step);
                                }
                              } else {
                                if(event.window_position.x < root.x + edge) {
                                  (void)style.scroll_controller.ScrollBy(-style.auto_scroll_step);
                                } else if(event.window_position.x > root.x + root.width - edge) {
                                  (void)style.scroll_controller.ScrollBy(style.auto_scroll_step);
                                }
                              }
                            }
                            drag = next;
                          })
                      .On<DragEvents::Ended>(
                          [drag, index, on_reordered](const DragEvent&) {
                            const DragSession finished = drag.Get();
                            if(!finished.active || finished.from != index) return;
                            if(on_reordered && finished.target != index) {
                              on_reordered(finished.from, finished.target);
                            }
                            drag = DragSession{};
                          })
                      .On<DragEvents::Canceled>(
                          [drag, index](const DragEvent&) {
                            const DragSession finished = drag.Get();
                            if(finished.active && finished.from == index) drag = DragSession{};
                          });
      if(style.keyboard_reorder) {
        slot = std::move(slot)
                   .With(Focusable{true})
                   .On<ViewEvents::KeyIntercept>(
                       [index, count, axis = style.axis, on_reordered](const KeyEvent& event) {
                         if(event.type != KeyEventType::Down || event.repeat || !event.modifiers.alt) return false;
                         std::optional<int> delta;
                         if(axis == Axis::Vertical) {
                           if(event.key == Key::ArrowUp) delta = -1;
                           if(event.key == Key::ArrowDown) delta = 1;
                         } else {
                           if(event.key == Key::ArrowLeft) delta = -1;
                           if(event.key == Key::ArrowRight) delta = 1;
                         }
                         if(!delta.has_value()) return false;
                         const long target = static_cast<long>(index) + *delta;
                         if(target < 0 || target >= static_cast<long>(count)) return false;
                         if(on_reordered) on_reordered(index, static_cast<std::size_t>(target));
                         return true;
                       });
      }
      slot = std::move(slot).With(Semantics{.role = SemanticRole::ListItem,
                                           .collection_item = SemanticCollectionItem{.index = index}});
      if(session.active && session.target == index && session.from != index) {
        slot = std::move(slot).With(
            Border{.color = style.target_highlight, .width = style.target_highlight_width});
      }
      slots.push_back(std::move(slot));
    }
    View list = style.axis == Axis::Vertical ? View(Column(std::move(slots))) : View(Row(std::move(slots)));
    return std::move(list)
        .With(Spacing(style.spacing), ReorderRootBounds{root_bounds.Get()},
              Semantics{.role = SemanticRole::List,
                        .collection = SemanticCollection{.item_count = count}});
  });
}

} // namespace huxerui::interaction
