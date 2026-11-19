#pragma once

#include <huxerui/animation.h>
#include <huxerui/event.h>
#include <huxerui/geometry.h>
#include <huxerui/view.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace huxerui::interaction {
namespace detail {
struct SwipeDeckControllerState;
} // namespace detail

struct SwipeDeckStyle;

/// Discrete cardinal swipe direction resolved for a card in SwipeDeck.
enum class SwipeDirection {
  None,
  Left,
  Right,
  Up,
  Down,
};

/// Bitmask controlling which card swipe directions SwipeDeck recognizes.
enum class SwipeDirectionMask : std::uint32_t {
  None = 0U,
  Left = 1U << 0U,
  Right = 1U << 1U,
  Up = 1U << 2U,
  Down = 1U << 3U,
  Horizontal = Left | Right,
  Vertical = Up | Down,
  All = Horizontal | Vertical,
};

[[nodiscard]] constexpr SwipeDirectionMask operator|(SwipeDirectionMask a, SwipeDirectionMask b) noexcept {
  return static_cast<SwipeDirectionMask>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

[[nodiscard]] constexpr SwipeDirectionMask operator&(SwipeDirectionMask a, SwipeDirectionMask b) noexcept {
  return static_cast<SwipeDirectionMask>(static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b));
}

[[nodiscard]] constexpr bool HasDirection(SwipeDirectionMask mask, SwipeDirectionMask flag) noexcept {
  return (static_cast<std::uint32_t>(mask) & static_cast<std::uint32_t>(flag)) == static_cast<std::uint32_t>(flag)
         && static_cast<std::uint32_t>(flag) != 0U;
}

/// Live deck state supplied to each card builder in headless SwipeDeck().
struct SwipeDeckCardContext {
  /// Stable zero-based card index in `[0, count)`.
  std::size_t index = 0;
  /// Stack slot depth (`0` for the active top card, `1..visible_cards` behind it).
  std::size_t depth = 0;
  /// Convenience flag matching `depth == 0`.
  bool is_top = false;
  /// True while the user is actively dragging the top card (only true on `depth == 0`).
  bool dragging = false;
  /// Top card threshold progress in `[0, 1]`, useful both for decision overlays on `depth == 0` and promoting
  /// background cards cleanly.
  float progress = 0.0F;
  /// Top card translation divided by `swipe_threshold`. Zero for non-top cards.
  Point normalized_offset{};
  /// Leading cardinal direction of the top card's current drag/exit motion (`None` at rest).
  SwipeDirection direction = SwipeDirection::None;

  bool operator==(const SwipeDeckCardContext&) const = default;
};

/// Copyable handle for observing and programmatically swiping, rewinding, or resetting a SwipeDeck.
/// Default-constructed handles are unbound and safe outside an Application runtime; bind one with
/// UseSwipeDeckController().
class SwipeDeckController {
public:
  SwipeDeckController() noexcept = default;

  [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(state_); }

  /// Current active card index at the top of the deck (subscribes active composition).
  [[nodiscard]] std::size_t TopIndex() const;

  /// Number of unswiped cards remaining in the deck (subscribes active composition).
  [[nodiscard]] std::size_t RemainingCount() const;

  /// True while the user is dragging the top card (subscribes active composition).
  [[nodiscard]] bool IsDragging() const;

  /// True when at least one swiped card exists in the rewind history (subscribes active composition).
  [[nodiscard]] bool CanRewind() const;

  /// Current threshold progress in `[0, 1]` of the top card (subscribes active composition).
  [[nodiscard]] float DragProgress() const;

  /// Current leading cardinal direction of the top card (`None` at rest; subscribes active composition).
  [[nodiscard]] SwipeDirection DragDirection() const;

  /// Programmatically flings the current top card out toward `direction` (must not be `SwipeDirection::None`).
  bool Swipe(SwipeDirection direction) const;

  /// Smoothly brings the most recently swiped card back onto the top of the deck.
  bool Rewind() const;

  /// Immediately clears swipe history, resets TopIndex to 0, and returns the deck to rest.
  void Reset() const;

  bool operator==(const SwipeDeckController&) const = default;

private:
  explicit SwipeDeckController(std::shared_ptr<detail::SwipeDeckControllerState> state) : state_(std::move(state)) {}

  std::shared_ptr<detail::SwipeDeckControllerState> state_;

  friend SwipeDeckController UseSwipeDeckController(std::size_t initial_index);
  friend View SwipeDeck(std::size_t count, std::function<View(SwipeDeckCardContext)> card_builder,
                        std::function<void(std::size_t index, SwipeDirection direction)> on_swiped,
                        SwipeDeckStyle style, std::function<View()> empty_builder);
};

/// Remembers a SwipeDeckController in the current composition scope.
SwipeDeckController UseSwipeDeckController(std::size_t initial_index = 0);

/// Layout, physics, and stack visual configuration for SwipeDeck.
struct SwipeDeckStyle {
  /// Number of visible stacked layers behind/including the top card (`>= 1`).
  std::size_t visible_cards = 3;
  /// Permitted swipe directions (`Horizontal` avoids intercepting vertical ScrollView drags). Must be non-empty.
  SwipeDirectionMask allowed_directions = SwipeDirectionMask::Horizontal;
  /// Displacement in logical pixels required to commit a swipe without high flick velocity. Must be finite and > 0.
  float swipe_threshold = 110.0F;
  /// Release velocity in logical pixels per second required to commit a fast flick. Must be finite and >= 0.
  float minimum_velocity = 550.0F;
  /// Exit travel distance in logical pixels when committing or calling Swipe()/Rewind(). Must be finite and > 0.
  float exit_distance = 440.0F;
  /// Maximum card rotation tilt in degrees reached at `swipe_threshold` horizontal drag. Must be finite and >= 0.
  float maximum_rotation = 12.0F;
  /// Scale decrement applied per background stack depth step. Must be finite and in `[0, 0.5)`.
  float depth_scale_step = 0.05F;
  /// Vertical offset step per background depth layer in logical pixels. Must be finite.
  float depth_offset_y = 12.0F;
  /// Opacity decrement applied per background depth step. Must be finite and in `[0, 1]`.
  float depth_opacity_step = 0.12F;
  /// Maximum release velocity magnitude handed off into return/exit animations. Must be finite and >= 0.
  float maximum_velocity = 2800.0F;
  /// Spring motion when a sub-threshold drag snaps back to center.
  AnimationSpec return_motion = SpringSpec{.stiffness = 380.0F, .damping_ratio = 0.75F};
  /// Exit animation when committing a swipe out of the deck.
  AnimationSpec exit_motion = TweenSpec{.duration = 0.24, .easing = Easing::EaseOut};
  /// Return animation when Rewind() flies the previous card back onto the top of the deck.
  AnimationSpec rewind_motion = SpringSpec{.stiffness = 320.0F, .damping_ratio = 0.82F};
  /// Cursor displayed over the draggable top card.
  PointerCursorKind cursor = PointerCursorKind::Grab;
  /// Optional external controller bound to this SwipeDeck.
  SwipeDeckController controller{};

  bool operator==(const SwipeDeckStyle&) const = default;
};

/// Headless stacked swipeable card deck invoking `card_builder(SwipeDeckCardContext)` for visible cards.
/// @throws std::invalid_argument If `count > 0 && !card_builder` or any `style` value is out of range.
View SwipeDeck(std::size_t count, std::function<View(SwipeDeckCardContext)> card_builder,
               std::function<void(std::size_t index, SwipeDirection direction)> on_swiped = {},
               SwipeDeckStyle style = {}, std::function<View()> empty_builder = {});

/// Convenience SwipeDeck overload stacked over prebuilt `cards`.
/// @throws std::invalid_argument If any entry in `cards` is empty or `style` is invalid.
View SwipeDeck(std::vector<View> cards, std::function<void(std::size_t index, SwipeDirection direction)> on_swiped = {},
               SwipeDeckStyle style = {}, View empty_view = {});

} // namespace huxerui::interaction
