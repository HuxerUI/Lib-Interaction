#pragma once

#include <huxerui/animation.h>
#include <huxerui/event.h>
#include <huxerui/geometry.h>
#include <huxerui/view.h>

#include <functional>

namespace huxerui::interaction {

/// Presentation and threshold configuration for SwipeActions.
struct SwipeActionsStyle {
  /// Width of the trailing action strip revealed by a leftward swipe, in logical pixels. Must be finite and positive.
  float action_extent = 88.0F;
  /// Fraction of action_extent that must be exposed on release before the strip settles open. Must be in [0, 1].
  float activation_ratio = 0.45F;
  /// Leftward release velocity, in logical pixels per second, that opens the strip even below the distance threshold.
  /// Must be finite and non-negative.
  float minimum_velocity = 400.0F;
  /// Motion used while the pointer drags the content.
  AnimationSpec follow_motion = TweenSpec{.duration = 0.04, .easing = Easing::Linear};
  /// Motion used when the content settles open or closed.
  AnimationSpec settle_motion = TweenSpec{.duration = 0.2, .easing = Easing::EaseOut};
  bool operator==(const SwipeActionsStyle&) const = default;
};

/// Composes content with a trailing action strip revealed by a horizontal swipe.
///
/// The action builder runs during composition and receives a close callback. Return a view that fills the
/// style.action_extent width; the widget gives it the content height and clips the whole surface. Actions remain
/// behind the content and become clickable once the content slides away. The content follows the pointer with
/// follow_motion; on release it settles open when the exposed fraction or leftward velocity crosses the threshold,
/// otherwise it settles closed. A canceled drag always closes.
///
/// Call during composition. Each call creates an independent state scope; recomposition with equal arguments
/// preserves the open/drag state by scope position. The widget imposes no intrinsic frame; supply bounds through a
/// parent or Frame.
/// @param content Content displayed above the action strip.
/// @param actions Builder for the trailing action strip. It must not be empty and must return a view that fills the
///     supplied extent and height.
/// @param style Extent, threshold, velocity, and motions. Must satisfy the documented ranges.
/// @return A View whose mounted instance owns the swipe offset and drag session.
/// @throws std::invalid_argument If the style is invalid or actions is empty.
/// @code
/// SwipeActions(
///     ListRow{...},
///     [](std::function<void()> close) {
///       return Button("Delete").With(Background{Color::Rgb(180, 50, 50)})
///           .OnClick([close] { Delete(); close(); });
///     })
///     .With(Frame{.width = 320.0F, .height = 64.0F});
/// @endcode
View SwipeActions(View content, std::function<View(std::function<void()>)> actions,
                  SwipeActionsStyle style = {});

} // namespace huxerui::interaction
