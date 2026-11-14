#pragma once

#include <huxerui/animation.h>
#include <huxerui/color.h>
#include <huxerui/event.h>
#include <huxerui/gesture.h>
#include <huxerui/geometry.h>
#include <huxerui/view.h>

#include <chrono>
#include <functional>
#include <optional>

namespace huxerui::interaction {

/// Presentation configuration for LongPressable. Recognition follows the platform GestureSettings unless overridden
/// per widget; the presentation fields control only the hold feedback.
struct LongPressableStyle {
  /// Time the pointer must remain within the movement tolerance before recognition. An empty value uses the
  /// platform default long press duration. Must be positive when set.
  std::optional<std::chrono::duration<double>> minimum_duration = std::nullopt;
  /// Node-local movement tolerance override in logical pixels. An empty value uses the platform pointer slop.
  /// Must be finite and positive when set.
  std::optional<float> maximum_movement = std::nullopt;
  /// Scale applied while a recognized long press owns the pointer. Rest state is 1.0. Must be finite and positive.
  float held_scale = 0.97F;
  /// Motion used when the scale target changes, including the return to rest.
  AnimationSpec animation = TweenSpec{.duration = 0.16, .easing = Easing::EaseOut};
  /// Pivot used by the scale transform. Defaults to the center of the content.
  TransformOrigin origin = {};
  /// Cursor shown over the content. Default keeps the platform cursor.
  PointerCursorKind cursor = PointerCursorKind::Default;
  /// Whether to draw a circular progress ring over the content while the hold runs. The ring fills across the
  /// resolved hold duration — minimum_duration when set, otherwise the framework's 0.5 s default — and the same
  /// resolved value is applied to the gesture so the ring and recognition stay synchronized.
  bool show_progress = false;
  /// Ring color; the background track uses this color at reduced alpha. Used only when show_progress is set.
  Color progress_color = Color::Rgb(255, 255, 255, 0.9F);
  bool operator==(const LongPressableStyle&) const = default;
};

/// Composes content that recognizes a press-and-hold gesture and reports recognition through the callback.
///
/// The gesture wins the pointer sequence once the hold duration elapses within the movement tolerance; the callback
/// fires at recognition, the content scales to held_scale while the press is owned, and the sequence ends on
/// pointer up or cancel. With LongPressableStyle::show_progress the widget also paints a circular ring that fills
/// across the hold duration and drains on early release. OnClick and other gestures stay composable at the call
/// site.
///
/// Call during composition. Each call creates an independent state scope; recomposition with equal arguments
/// preserves the mounted hold state by scope position. The widget imposes no intrinsic frame.
/// @param content Content displayed inside the interactive surface.
/// @param style Recognition overrides and hold presentation. Must satisfy the documented ranges.
/// @param on_long_press Callback invoked once when the long press is recognized, or an empty function to compose a
///     purely visual hold surface.
/// @return A View whose mounted instance owns the hold state.
/// @throws std::invalid_argument If the duration override is not positive, the movement override is not finite and
///     positive, or the held scale is non-finite or not positive.
/// @code
/// LongPressable(Card(), {.minimum_duration = std::chrono::duration<double>(0.4)}, onDelete)
///     .With(Frame{.width = 120.0F, .height = 120.0F});
/// @endcode
View LongPressable(View content, LongPressableStyle style = {}, std::function<void()> on_long_press = {});

} // namespace huxerui::interaction
