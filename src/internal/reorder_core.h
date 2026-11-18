#pragma once

#include <huxerui/environment.h>
#include <huxerui/event.h>
#include <huxerui/geometry.h>
#include <huxerui/gesture.h>
#include <huxerui/modifier.h>
#include <huxerui/scroll.h>
#include <huxerui/state.h>
#include <huxerui/view.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>

namespace huxerui::interaction::internal {

/// Shared slot geometry recorded during layout preparation, keyed by slot index.
/// Holds undragged layout positions in window coordinates so hit testing remains stable while items animate.
class SlotGeometry {
public:
  void Record(std::size_t index, Rect window_bounds);
  [[nodiscard]] std::optional<Rect> Find(std::size_t index) const;
  [[nodiscard]] std::optional<std::size_t> Closest(std::size_t count, Point window_position) const;

private:
  std::unordered_map<std::size_t, Rect> slots_;
};

/// Retained extension recording one slot's undragged window-space bounds on every geometry pass.
struct SlotBounds {
  class Extension;
  static const huxerui::detail::ModifierDescriptor& Descriptor();

  std::shared_ptr<SlotGeometry> geometry;
  std::size_t index = 0;

  bool operator==(const SlotBounds&) const = default;
};

/// Retained extension recording the container's window-space bounds for edge auto-scroll.
struct RootBounds {
  class Extension;
  static const huxerui::detail::ModifierDescriptor& Descriptor();

  std::shared_ptr<Rect> bounds;

  bool operator==(const RootBounds&) const = default;
};

/// One mounted drag session shared by every slot in a Reorderable or ReorderableGrid.
struct ReorderSession {
  bool active = false;
  std::size_t from = 0;
  std::size_t target = 0;
  Point translation{};

  bool operator==(const ReorderSession&) const = default;
};

/// Shared mutable configuration and state referenced by slots and nested ReorderHandles without reallocating per frame.
struct ReorderContext {
  State<ReorderSession> drag;
  std::shared_ptr<SlotGeometry> geometry = std::make_shared<SlotGeometry>();
  std::shared_ptr<Rect> root_bounds = std::make_shared<Rect>();
  std::size_t count = 0;
  std::optional<float> auto_scroll_edge;
  float auto_scroll_step = 24.0F;
  ScrollController scroll_controller{};
  std::function<void(std::size_t, std::size_t)> on_reordered;
};

/// Inherited environment binding letting a nested ReorderHandle drive its enclosing slot's drag session.
struct ReorderSlotEnvironment {
  std::shared_ptr<ReorderContext> context;
  std::size_t index = 0;

  static ReorderSlotEnvironment Default() { return {}; }
  bool operator==(const ReorderSlotEnvironment&) const = default;
};

/// Computes the slot index occupied by `index` while `from` previews `to`.
[[nodiscard]] std::size_t DisplacedIndex(std::size_t index, std::size_t from, std::size_t to) noexcept;

/// Computes the visual offset and active motion for one slot during or outside a drag session.
[[nodiscard]] Point ComputeSlotOffset(const ReorderSession& session, const SlotGeometry& geometry,
                                      std::size_t index);

/// Attaches the drag gesture and lifecycle callbacks that drive `context` for `index`.
[[nodiscard]] View BindReorderDrag(View view, const std::shared_ptr<ReorderContext>& context, std::size_t index,
                                   DragGesture gesture = {});

/// Attaches Alt + arrow keyboard reordering along `axis` (or 2D arrow step when `two_dimensional` is set).
[[nodiscard]] View BindKeyboardReorder(View view, const std::shared_ptr<ReorderContext>& context, std::size_t index,
                                       Axis axis, bool two_dimensional = false);

} // namespace huxerui::interaction::internal
