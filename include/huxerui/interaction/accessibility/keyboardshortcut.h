#pragma once

#include <huxerui/event.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <functional>
#include <utility>

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
/// @throws std::invalid_argument If `chord.key` is `Key::Unknown`.
View KeyboardShortcut(View content, KeyChord chord, std::function<void()> on_trigger,
                      KeyboardShortcutStyle style = {});

/// Pipeable adaptor for KeyboardShortcut.
[[nodiscard]] inline ViewAdaptor KeyboardShortcut(KeyChord chord, std::function<void()> on_trigger,
                                                  KeyboardShortcutStyle style = {}) {
  return ViewAdaptor([chord, on_trigger = std::move(on_trigger), style](View content) {
    return KeyboardShortcut(std::move(content), chord, on_trigger, style);
  });
}

} // namespace huxerui::interaction
