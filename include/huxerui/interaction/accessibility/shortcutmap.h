#pragma once

#include <huxerui/event.h>
#include <huxerui/interaction/accessibility/keyboardshortcut.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <functional>
#include <utility>
#include <vector>

namespace huxerui::interaction {

/// Single shortcut binding inside a ShortcutMap.
struct ShortcutEntry {
  /// Portable key chord activating this entry. Must not be `Key::Unknown`.
  KeyChord chord{};
  /// Callback invoked when a matching non-suppressed key press arrives.
  std::function<void()> on_trigger{};
  /// Whether matching events stop propagating after running `on_trigger`.
  bool consume = true;
  /// Whether auto-repeated key-down events also trigger this binding.
  bool allow_repeat = false;
};

/// Behavior configuration for ShortcutMap.
struct ShortcutMapStyle {
  /// Enables or disables all entries in this ShortcutMap without unregistering the modifier.
  bool enabled = true;

  bool operator==(const ShortcutMapStyle&) const = default;
};

/// Binds multiple key chords through a single `KeyIntercept` handler on an active focus route so entries do not
/// overwrite one another on the same View.
/// @throws std::invalid_argument If any entry specifies `Key::Unknown`.
View ShortcutMap(View content, std::vector<ShortcutEntry> entries, ShortcutMapStyle style = {});

/// Pipeable adaptor for ShortcutMap.
[[nodiscard]] inline ViewAdaptor ShortcutMap(std::vector<ShortcutEntry> entries, ShortcutMapStyle style = {}) {
  return ViewAdaptor([entries = std::move(entries), style](View content) {
    return ShortcutMap(std::move(content), entries, style);
  });
}

} // namespace huxerui::interaction
