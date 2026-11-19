#pragma once

#include <huxerui/animation.h>
#include <huxerui/event.h>
#include <huxerui/geometry.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace huxerui::interaction {
namespace detail {
struct SnapDragControllerState;
} // namespace detail

struct SnapDragStyle;

/// Selects the target anchor set used by SnapDrag when a drag ends or SnapToAnchor() is called.
enum class SnapMode {
  /// Snaps to the four stage corners: `0 = TopLeft`, `1 = TopRight`, `2 = BottomLeft`, `3 = BottomRight`.
  Corners,
  /// Snaps horizontally to Left (`0`) or Right (`1`) edge while preserving vertical position.
  HorizontalEdges,
  /// Snaps vertically to Top (`0`) or Bottom (`1`) edge while preserving horizontal position.
  VerticalEdges,
  /// Snaps to the nearest normalized point in `SnapDragStyle::anchors`.
  Custom,
};

/// Live floating position and anchor state passed to the headless SnapDrag builder.
struct SnapDragSnapshot {
  /// True while a pointer drag sequence is active.
  bool dragging = false;
  /// Index of the current resting or snapped anchor target.
  std::size_t active_anchor = 0;
  /// Current local offset inside the SnapDrag stage in logical pixels.
  Point position{};
  /// Current position normalized into `[0, 1]` within the available stage travel bounds.
  Point normalized{};

  bool operator==(const SnapDragSnapshot&) const = default;
};

/// Copyable handle for observing or programmatically snapping a mounted SnapDrag stage.
/// Default-constructed handles are unbound and safe outside an Application runtime; bind one with
/// UseSnapDragController().
class SnapDragController {
public:
  SnapDragController() noexcept = default;

  [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(state_); }

  /// True while the floating element is being dragged (subscribes active composition).
  [[nodiscard]] bool IsDragging() const;

  /// Current offset in stage-local logical pixels (subscribes active composition).
  [[nodiscard]] Point Position() const;

  /// Current normalized position in `[0, 1]` along each travel axis (subscribes active composition).
  [[nodiscard]] Point NormalizedPosition() const;

  /// Current target anchor index (subscribes active composition).
  [[nodiscard]] std::size_t ActiveAnchor() const;

  /// Snaps to anchor `index` for the configured SnapMode, optionally animating with `snap_motion`.
  bool SnapToAnchor(std::size_t index, bool animated = true) const;

  /// Snaps to normalized stage coordinates in `[0, 1]`, updating position smoothly when `animated` is true.
  bool SnapToNormalized(Point normalized, bool animated = true) const;

  bool operator==(const SnapDragController&) const = default;

private:
  explicit SnapDragController(std::shared_ptr<detail::SnapDragControllerState> state) : state_(std::move(state)) {}

  std::shared_ptr<detail::SnapDragControllerState> state_;

  friend SnapDragController UseSnapDragController(std::size_t initial_anchor);
  friend View SnapDrag(std::function<View(SnapDragSnapshot)> builder, SnapDragStyle style,
                       std::function<void(std::size_t, Point)> on_snapped);
};

/// Remembers a SnapDragController in the current composition scope.
SnapDragController UseSnapDragController(std::size_t initial_anchor = 0);

/// Behavior and spring configuration for SnapDrag.
struct SnapDragStyle {
  /// Target anchor topology selected upon release.
  SnapMode mode = SnapMode::Corners;
  /// Normalized target coordinates in `[0, 1]^2` used when `mode == SnapMode::Custom`. Must be non-empty then.
  std::vector<Point> anchors{};
  /// Initial anchor index applied before any user interaction.
  std::size_t initial_anchor = 0;
  /// Inset kept between the floating item and stage boundaries in logical pixels. Must be finite and non-negative.
  float margin = 12.0F;
  /// Forward velocity projection horizon in seconds when choosing the nearest snap target. Must be finite and >= 0.
  float projection_time = 0.14F;
  /// Maximum handoff velocity magnitude in logical pixels per second. Must be finite and >= 0.
  float maximum_velocity = 2400.0F;
  /// Subtle scale multiplier applied while dragging. Must be finite and positive.
  float drag_scale = 1.04F;
  /// Clamps dragged coordinates inside the stage margin bounds while dragging.
  bool clamp_to_bounds = true;
  /// Motion curve used when snapping after release or via SnapDragController.
  AnimationSpec snap_motion = SpringSpec{.stiffness = 360.0F, .damping_ratio = 0.78F};
  /// Resting pointer cursor while hovering the draggable item (switches to Grabbing during drag).
  PointerCursorKind cursor = PointerCursorKind::Grab;
  /// Optional external controller bound to this SnapDrag stage.
  SnapDragController controller{};

  bool operator==(const SnapDragStyle&) const = default;
};

/// Headless snapping floating draggable stage delegating item visuals to `builder(SnapDragSnapshot)`.
/// The stage fills finite parent bounds without consuming clicks outside the draggable child.
/// @throws std::invalid_argument If `builder` is empty or any `style` field is out of range.
View SnapDrag(std::function<View(SnapDragSnapshot)> builder, SnapDragStyle style = {},
              std::function<void(std::size_t anchor, Point position)> on_snapped = {});

/// Wraps `content` inside a bounded corner/edge-snapping floating draggable stage.
/// @throws std::invalid_argument If any `style` field is out of range.
View SnapDrag(View content, SnapDragStyle style = {},
              std::function<void(std::size_t anchor, Point position)> on_snapped = {});

/// Pipeable adaptor for SnapDrag.
[[nodiscard]] inline ViewAdaptor SnapDrag(SnapDragStyle style = {},
                                          std::function<void(std::size_t anchor, Point position)> on_snapped = {}) {
  return ViewAdaptor([style = std::move(style), on_snapped = std::move(on_snapped)](View content) {
    return SnapDrag(std::move(content), style, on_snapped);
  });
}

} // namespace huxerui::interaction
