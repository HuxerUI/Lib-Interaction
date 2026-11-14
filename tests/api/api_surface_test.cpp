#include <huxerui/interaction.h>
#include <huxerui/huxerui.h>

#include <utility>

using namespace huxerui;
using namespace huxerui::interaction;

/// Compiles the umbrella header and links one symbol from every widget family.
int main() {
  View probe = Pressable(Text("probe"));
  probe = std::move(probe).With(Ripple{});

  (void)HoverLift(Text("hover"));
  (void)Spotlight{};
  (void)Pulsing(Text("pulse"));
  (void)Shimmer{};
  (void)DragSpring(Text("drag"));
  (void)PinchZoom(Text("pinch"));
  (void)Reorderable({Text("a"), Text("b")});
  (void)ReorderableGrid({Text("a")});
  (void)FileDropZone(Text("files"), {});
  (void)ScrollProgress(ScrollController{});
  (void)ScrollEdgeFade(ScrollController{});
  (void)ContextMenuArea(Text("context"), {});
  (void)HoverCard(Text("hover"), [](PopupContext) { return Text("card"); });
  (void)KeyboardShortcut(Text("keys"), {.key = Key::K}, [] {});

  return probe ? 0 : 1;
}
