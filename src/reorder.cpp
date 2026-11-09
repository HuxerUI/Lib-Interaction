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
    auto drag = UseState(DragSession{});
    const DragSession session = drag.Get();
    const std::size_t count = items.size();
    const auto shared_geometry = geometry.Get();
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
                          [drag, index, shared_geometry, count](const DragEvent& event) {
                            DragSession next = drag.Get();
                            if(!next.active || next.from != index) return;
                            next.translation = event.translation;
                            if(const auto closest = shared_geometry->Closest(count, event.window_position)) {
                              next.target = *closest;
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
      if(session.active && session.target == index && session.from != index) {
        slot = std::move(slot).With(
            Border{.color = style.target_highlight, .width = style.target_highlight_width});
      }
      slots.push_back(std::move(slot));
    }
    return Column(std::move(slots)).With(Spacing(style.spacing));
  });
}

} // namespace huxerui::interaction
