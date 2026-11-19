#include <huxerui/interaction/accessibility/shortcutmap.h>

#include <stdexcept>
#include <utility>

namespace huxerui::interaction {

View ShortcutMap(View content, std::vector<ShortcutEntry> entries, ShortcutMapStyle style) {
  for(const ShortcutEntry& entry : entries) {
    if(entry.chord.key == Key::Unknown) {
      throw std::invalid_argument("HuxerUI Interaction: ShortcutMap requires known keys for every entry");
    }
  }
  View host = std::move(content);
  return std::move(host).On<ViewEvents::KeyIntercept>(
      [entries = std::move(entries), style](const KeyEvent& event) -> bool {
        if(!style.enabled || event.type != KeyEventType::Down) {
          return false;
        }
        for(const ShortcutEntry& entry : entries) {
          if(event.repeat && !entry.allow_repeat) continue;
          if(event.key == entry.chord.key && event.modifiers == entry.chord.modifiers) {
            if(entry.on_trigger) entry.on_trigger();
            return entry.consume;
          }
        }
        return false;
      });
}

} // namespace huxerui::interaction
