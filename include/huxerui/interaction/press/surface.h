#pragma once

#include <huxerui/event.h>
#include <huxerui/semantics.h>
#include <huxerui/view.h>

#include <concepts>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

namespace huxerui::interaction {

/// Live interaction flags observed by InteractiveSurface and passed to headless builders.
struct InteractionSnapshot {
  /// True while a hover-capable pointer is inside the surface bounds.
  bool hovered = false;
  /// True while a primary pointer press or focused Space key press is active.
  bool pressed = false;
  /// True while the surface holds keyboard focus.
  bool focused = false;

  bool operator==(const InteractionSnapshot&) const = default;
};

/// Behavioral options for InteractiveSurface, independent of visual styling.
struct InteractiveSurfaceOptions {
  /// Cursor shown over the surface. Default shows the hand cursor.
  PointerCursorKind cursor = PointerCursorKind::Hand;
  /// Whether an actionable surface participates in keyboard focus and Space/Enter activation.
  bool keyboard_focusable = true;
  /// Semantic role published when `on_click` is non-empty. Empty suppresses automatic role assignment.
  std::optional<SemanticRole> role = SemanticRole::Button;

  bool operator==(const InteractiveSurfaceOptions&) const = default;
};

/// Headless interactive surface that tracks hover, primary press, focus, and keyboard Space activation,
/// delegating all visual presentation to `builder`.
///
/// @param builder Invoked during composition with the current InteractionSnapshot. Must not be empty.
/// @param on_click Optional activation callback triggered by pointer click or Enter/Space.
/// @param options Cursor, keyboard focus participation, and semantic role.
/// @return A View whose mounted scope owns the interaction state machine.
/// @throws std::invalid_argument If `builder` is empty.
View InteractiveSurface(std::function<View(const InteractionSnapshot&)> builder,
                        std::function<void()> on_click = {},
                        InteractiveSurfaceOptions options = {});

/// Convenience overload accepting `options` without an activation callback.
inline View InteractiveSurface(std::function<View(const InteractionSnapshot&)> builder,
                               InteractiveSurfaceOptions options) {
  return InteractiveSurface(std::move(builder), {}, options);
}

} // namespace huxerui::interaction
