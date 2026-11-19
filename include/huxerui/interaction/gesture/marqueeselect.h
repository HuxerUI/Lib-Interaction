#pragma once

#include <huxerui/color.h>
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
struct MarqueeSelectControllerState;
} // namespace detail

struct MarqueeSelectStyle;

/// Copyable handle for observing or programmatically manipulating a MarqueeSelectArea selection.
/// Default-constructed handles are unbound and safe outside an Application runtime; bind one with
/// UseMarqueeSelectController().
class MarqueeSelectController {
public:
  MarqueeSelectController() noexcept = default;

  [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(state_); }

  /// True while a marquee box drag is active (subscribes active composition).
  [[nodiscard]] bool IsDragging() const;

  /// Sorted, deduplicated list of currently selected item indices (subscribes active composition).
  [[nodiscard]] std::vector<std::size_t> Selection() const;

  /// Convenience predicate reporting whether `index` is currently selected (subscribes active composition).
  [[nodiscard]] bool IsSelected(std::size_t index) const;

  /// Replaces the current selection with a sorted, deduplicated copy of `indices`.
  void SetSelection(std::vector<std::size_t> indices) const;

  /// Toggles `index` in the current selection.
  void Toggle(std::size_t index) const;

  /// Clears all selected indices.
  void Clear() const;

  bool operator==(const MarqueeSelectController&) const = default;

private:
  explicit MarqueeSelectController(std::shared_ptr<detail::MarqueeSelectControllerState> state)
      : state_(std::move(state)) {}

  std::shared_ptr<detail::MarqueeSelectControllerState> state_;

  friend MarqueeSelectController UseMarqueeSelectController(std::vector<std::size_t> initial_selection);
  friend View MarqueeSelectArea(View content, MarqueeSelectStyle style,
                                std::function<void(const std::vector<std::size_t>&)> on_selection_changed,
                                std::function<void(const std::vector<std::size_t>&)> on_selection_committed);
};

/// Remembers a MarqueeSelectController in the current composition scope.
MarqueeSelectController UseMarqueeSelectController(std::vector<std::size_t> initial_selection = {});

/// Visual and behavioral configuration for MarqueeSelectArea.
struct MarqueeSelectStyle {
  /// Translucent fill color painted inside the active drag rectangle.
  Color fill_color = Color::Rgb(31, 111, 235, 0.16F);
  /// Border color stroked around the active drag rectangle.
  Color border_color = Color::Rgb(31, 111, 235, 0.85F);
  /// Border width in logical pixels (0 disables). Must be finite and non-negative.
  float border_width = 1.5F;
  /// Corner radii of the drag rectangle. All four corners must be finite and non-negative.
  CornerRadii corner_radii{4.0F};
  /// Optional drag distance threshold in logical pixels before marquee selection begins. Must be finite and positive.
  std::optional<float> minimum_distance = std::nullopt;
  /// When true, clamps the selection box within the MarqueeSelectArea bounds.
  bool clamp_to_bounds = true;
  /// When true, unions box hits with the selection present when dragging started.
  bool additive = false;
  /// Optional external controller bound to this MarqueeSelectArea.
  MarqueeSelectController controller{};

  bool operator==(const MarqueeSelectStyle&) const = default;
};

/// Wraps `content` in a drag-box multi-selection container that tracks intersecting MarqueeSelectItem descendants
/// and renders an overlaid rubber-band rectangle while dragging.
/// @throws std::invalid_argument If any numeric style field is invalid.
View MarqueeSelectArea(View content, MarqueeSelectStyle style = {},
                       std::function<void(const std::vector<std::size_t>&)> on_selection_changed = {},
                       std::function<void(const std::vector<std::size_t>&)> on_selection_committed = {});

/// Pipeable adaptor for MarqueeSelectArea.
[[nodiscard]] inline ViewAdaptor MarqueeSelectArea(
    MarqueeSelectStyle style = {}, std::function<void(const std::vector<std::size_t>&)> on_selection_changed = {},
    std::function<void(const std::vector<std::size_t>&)> on_selection_committed = {}) {
  return ViewAdaptor([style, on_selection_changed = std::move(on_selection_changed),
                      on_selection_committed = std::move(on_selection_committed)](View content) {
    return MarqueeSelectArea(std::move(content), style, on_selection_changed, on_selection_committed);
  });
}

/// Registers a child view at `index` inside the enclosing MarqueeSelectArea so its live presentation bounds
/// participate in rubber-band intersection tests.
View MarqueeSelectItem(View content, std::size_t index);

/// Pipeable adaptor for MarqueeSelectItem.
[[nodiscard]] inline ViewAdaptor MarqueeSelectItem(std::size_t index) {
  return ViewAdaptor([index](View content) { return MarqueeSelectItem(std::move(content), index); });
}

} // namespace huxerui::interaction
