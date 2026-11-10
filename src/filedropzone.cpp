#include <huxerui/interaction/filedropzone.h>

#include <huxerui/modifier.h>
#include <huxerui/state.h>

#include <cmath>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const FileDropZoneStyle& style) {
  if(!std::isfinite(style.border_width) || style.border_width < 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: FileDropZone border width must be finite and non-negative");
  }
}

} // namespace

View FileDropZone(View content, FileDropOptions options,
                  std::function<void(const std::vector<FileReference>&, const FileDropEvent&)> on_dropped,
                  FileDropZoneStyle style) {
  ValidateStyle(style);
  const FileDropTarget target = FileDropTarget::Accepts(std::move(options));
  return Scope([content = std::move(content), target, on_dropped = std::move(on_dropped), style]() -> View {
    auto hovered = UseState(false);
    View zone = content;
    zone = std::move(zone)
               .With(target)
               .On<FileDropEvents::Entered>([hovered](const FileDropOffer&, const FileDropEvent&) {
                 hovered = true;
               })
               .On<FileDropEvents::Moved>([hovered](const FileDropOffer&, const FileDropEvent&) {
                 hovered = true;
               })
               .On<FileDropEvents::Exited>([hovered](const FileDropOffer&, const FileDropEvent&) {
                 hovered = false;
               })
               .On<FileDropEvents::Dropped>(
                   [hovered, on_dropped](const std::vector<FileReference>& files, const FileDropEvent& event) {
                     hovered = false;
                     if(on_dropped) on_dropped(files, event);
                   })
               .On<FileDropEvents::Failed>([hovered](const IoError&, const FileDropEvent&) {
                 hovered = false;
               })
               .With(Border{hovered.Get() ? style.highlight_color : Color::Transparent(),
                            hovered.Get() ? style.border_width : 0.0F});
    return zone;
  });
}

} // namespace huxerui::interaction
