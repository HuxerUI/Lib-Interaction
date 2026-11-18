#include <huxerui/interaction/dragdrop/filedropzone.h>

#include "../internal/validate.h"

#include <huxerui/modifier.h>
#include <huxerui/state.h>

#include <stdexcept>
#include <utility>

namespace huxerui::interaction {

View FileDropZone(std::function<View(bool hovered)> builder, FileDropOptions options,
                  std::function<void(const std::vector<FileReference>&, const FileDropEvent&)> on_dropped) {
  if(!builder) {
    throw std::invalid_argument("HuxerUI Interaction: FileDropZone requires a view builder");
  }
  const FileDropTarget target = FileDropTarget::Accepts(std::move(options));
  return Scope([builder = std::move(builder), target, on_dropped = std::move(on_dropped)]() -> View {
    auto hovered = UseState(false);
    View zone = builder(hovered.Get());
    zone = std::move(zone)
               .With(target)
               .On<FileDropEvents::Entered>([hovered](const FileDropOffer&, const FileDropEvent&) { hovered = true; })
               .On<FileDropEvents::Moved>([hovered](const FileDropOffer&, const FileDropEvent&) { hovered = true; })
               .On<FileDropEvents::Exited>([hovered](const FileDropOffer&, const FileDropEvent&) { hovered = false; })
               .On<FileDropEvents::Dropped>(
                   [hovered, on_dropped](const std::vector<FileReference>& files, const FileDropEvent& event) {
                     hovered = false;
                     if(on_dropped) on_dropped(files, event);
                   })
               .On<FileDropEvents::Failed>([hovered](const IoError&, const FileDropEvent&) { hovered = false; });
    return zone;
  });
}

View FileDropZone(View content, FileDropOptions options,
                  std::function<void(const std::vector<FileReference>&, const FileDropEvent&)> on_dropped,
                  FileDropZoneStyle style) {
  internal::RequireNonNegative(style.border_width,
                               "HuxerUI Interaction: FileDropZone border width must be finite and non-negative");
  return FileDropZone(
      [content = std::move(content), style](bool hovered) -> View {
        View zone = content;
        return std::move(zone).With(Border{
            hovered ? style.highlight_color : Color::Transparent(),
            hovered ? style.border_width : 0.0F,
        });
      },
      std::move(options), std::move(on_dropped));
}

} // namespace huxerui::interaction
