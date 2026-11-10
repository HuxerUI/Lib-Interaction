#pragma once

#include <huxerui/color.h>
#include <huxerui/file_drop.h>
#include <huxerui/geometry.h>
#include <huxerui/view.h>

#include <functional>
#include <vector>

namespace huxerui::interaction {

/// Presentation configuration for FileDropZone.
struct FileDropZoneStyle {
  /// Border color while an eligible external file offer targets the zone.
  Color highlight_color = Color::Rgb(31, 111, 235, 0.9F);
  /// Border width while highlighted, in logical pixels. Must be finite and non-negative.
  float border_width = 2.0F;
  bool operator==(const FileDropZoneStyle&) const = default;
};

/// Receives external files on content with a highlighted border.
///
/// The wrapper attaches FileDropTarget with the supplied extension/MIME/batch filters, tracks hover from
/// FileDropEvents::Entered/Moved/Exited, clears on Failed, and forwards a successful batch to on_dropped. File
/// references are borrowed for the callback; copy them to retain access. The zone imposes no intrinsic frame.
/// @param content Content that acts as the external drop target.
/// @param options File extension, MIME, and batch filters. Invalid filters throw immediately.
/// @param on_dropped Optional callback invoked once per accepted batch.
/// @param style Highlight color and border width.
/// @return A View carrying the FileDropTarget modifier and highlight presentation.
/// @throws std::invalid_argument If the filters or border width are invalid.
View FileDropZone(View content, FileDropOptions options,
                  std::function<void(const std::vector<FileReference>&, const FileDropEvent&)> on_dropped = {},
                  FileDropZoneStyle style = {});

} // namespace huxerui::interaction
