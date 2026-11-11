#pragma once

#include <huxerui/event.h>
#include <huxerui/geometry.h>
#include <huxerui/view.h>

#include <functional>

namespace huxerui::interaction {

/// Configuration for SecondaryClickable.
struct SecondaryClickableStyle {
  /// Cursor shown over the content.
  PointerCursorKind cursor = PointerCursorKind::Default;
  bool operator==(const SecondaryClickableStyle&) const = default;
};

/// Routes secondary-click and context-menu-key requests to a callback.
///
/// The wrapper uses ViewEvents::ContextMenuRequested, so it covers unchorded secondary taps, the Context Menu key, and
/// Shift+F10 without consuming ordinary clicks or pointer events. It does not synthesize a touch long press; pair it
/// with ContextMenuArea when touch needs the same request.
/// @param content Content receiving the request.
/// @param on_secondary Callback with window-local request position.
/// @param style Cursor configuration.
/// @return A View with the context-menu request binding.
View SecondaryClickable(View content, std::function<void(Point)> on_secondary,
                        SecondaryClickableStyle style = {});

} // namespace huxerui::interaction
