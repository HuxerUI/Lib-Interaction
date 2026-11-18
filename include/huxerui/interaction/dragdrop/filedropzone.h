#pragma once

#include <huxerui/color.h>
#include <huxerui/file_drop.h>
#include <huxerui/geometry.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <functional>
#include <utility>
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

/// Headless external FileDropZone overload delegating hover presentation to `builder(bool hovered)`.
/// @throws std::invalid_argument If `builder` is empty or `options` are malformed.
View FileDropZone(std::function<View(bool hovered)> builder, FileDropOptions options,
                  std::function<void(const std::vector<FileReference>&, const FileDropEvent&)> on_dropped = {});

/// Receives external files on content with a highlighted border.
/// @throws std::invalid_argument If the filters or border width are invalid.
View FileDropZone(View content, FileDropOptions options,
                  std::function<void(const std::vector<FileReference>&, const FileDropEvent&)> on_dropped = {},
                  FileDropZoneStyle style = {});

/// Pipeable adaptor for FileDropZone.
[[nodiscard]] inline ViewAdaptor FileDropZone(
    FileDropOptions options,
    std::function<void(const std::vector<FileReference>&, const FileDropEvent&)> on_dropped = {},
    FileDropZoneStyle style = {}) {
  return ViewAdaptor(
      [options = std::move(options), on_dropped = std::move(on_dropped), style](View content) {
        return FileDropZone(std::move(content), options, on_dropped, style);
      });
}

} // namespace huxerui::interaction
