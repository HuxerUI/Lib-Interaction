#include <huxerui/interaction/keyboardshortcut.h>

#include <stdexcept>
#include <utility>

namespace huxerui::interaction {

View KeyboardShortcut(View content, KeyChord chord, std::function<void()> on_trigger,
                      KeyboardShortcutStyle style) {
  if(chord.key == Key::Unknown) {
    throw std::invalid_argument("HuxerUI Interaction: KeyboardShortcut requires a known key");
  }
  View shortcut = std::move(content);
  return std::move(shortcut).On<ViewEvents::KeyIntercept>(
      [chord, on_trigger = std::move(on_trigger), style](const KeyEvent& event) -> bool {
        if(event.type != KeyEventType::Down || event.repeat || event.key != chord.key
           || !(event.modifiers == chord.modifiers)) {
          return false;
        }
        if(on_trigger) on_trigger();
        return style.consume;
      });
}

} // namespace huxerui::interaction
