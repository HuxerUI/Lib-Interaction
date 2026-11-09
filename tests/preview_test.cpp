#include "../examples/preview/src/app.h"
#include <huxerui/testing/ui_test.h>
#include "test.h"

using namespace huxerui;
using namespace huxerui::testing;

class PreviewResources final : public PlatformResources {
public:
  ResourceConfiguration Configuration() const override { return {}; }
  std::optional<InputStream> OpenRead(std::string_view path) override {
    auto stream = File(std::string(INTERACTION_TEST_RESOURCES) + "/" + std::string(path)).OpenRead();
    if(!stream.Succeeded()) return std::nullopt;
    return std::move(stream).Value();
  }
};

int main() { return Run([] {
  Application app(interaction_demo::PreviewApp, {
    .window = {.initial_size = {1100, 840}},
    .show_debug_overlay = false,
  });
  UiTest ui(app, {.viewport = {1100, 840}, .resource_provider = std::make_shared<PreviewResources>()});
  CHECK(ui.Find(UiSelector::Key("pressable-demo")).Exists());
  CHECK(ui.Find(UiSelector::Key("lift-demo")).Exists());
  CHECK(ui.Find(UiSelector::Key("pulse-demo")).Exists());
  CHECK(ui.Find(UiSelector::Key("ripple-demo")).Exists());
  CHECK(ui.Find(UiSelector::Key("spotlight-demo")).Exists());
  CHECK(ui.Find(UiSelector::Key("longpress-demo")).Exists());
  CHECK(ui.Find(UiSelector::Key("doubleclick-demo")).Exists());
  CHECK(ui.Find(UiSelector::Key("pop-demo")).Exists());
  CHECK(ui.Find(UiSelector::Key("shimmer-demo")).Exists());
  CHECK(ui.Find(UiSelector::Key("dragspring-demo")).Exists());
  CHECK(ui.Find(UiSelector::Text("clicks: 0")).Exists());
  ui.Find(UiSelector::Key("pressable-demo")).Tap();
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("clicks: 1")).Exists());
  ui.Find(UiSelector::Key("ripple-demo")).Tap();
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("clicks: 1")).Exists());
  ui.Find(UiSelector::Key("shake-demo")).Tap();
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Key("shake-demo")).Exists());
  ui.Find(UiSelector::Key("theme-dark")).Tap();
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Key("theme-system")).Exists());
  ui.Find(UiSelector::Key("theme-system")).Tap();
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Key("theme-light")).Exists());
  ui.Find(UiSelector::Key("pop-demo")).Tap();
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Key("longpress-demo")).Exists());
  ui.PumpAndSettle();
}); }
