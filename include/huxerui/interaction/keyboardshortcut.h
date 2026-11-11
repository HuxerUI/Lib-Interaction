#pragma once

#include <huxerui/event.h>
#include <huxerui/view.h>

#include <functional>

namespace huxerui::interaction {

/// Identifies one key and exact modifier state.
struct KeyChord {
  /// Portable key identity.
  Key key = Key::Unknown;
  /// Required modifier state, including exact false fields.
  KeyModifiers modifiers{};
  bool operator==(const KeyChord&) const = default;
};

/// Configuration for KeyboardShortcut.
struct KeyboardShortcutStyle {
  /// Whether a matching key event is consumed after the callback runs.
  bool consume = true;
  bool operator==(const KeyboardShortcutStyle&) const = default;
};

/// Binds one key chord on an active focus route.
///
/// The binding uses ViewEvents::KeyIntercept, so it runs before the focused component and before UiWindow defaults.
/// Mount the wrapper on the focus-scope root for application shortcuts, or around a focused control for local
/// shortcuts. The chord matches exactly; omitted modifier fields mean "not pressed".
/// @param content Content receiving the focus-route binding.
/// @param chord Key and exact modifiers to match.
/// @param on_trigger Callback invoked on a matching initial Down event.
/// @param style Whether to consume the event after the callback.
/// @return A View with the KeyIntercept binding attached.
/// @throws std::invalid_argument If chord.key is Unknown.
View KeyboardShortcut(View content, KeyChord chord, std::function<void()> on_trigger,
                      KeyboardShortcutStyle style = {});

} // namespace huxerui::interaction
