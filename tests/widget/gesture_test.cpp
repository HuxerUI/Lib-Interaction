#include <huxerui/interaction.h>
#include <huxerui/huxerui.h>

#include "../support/test.h"

#include <chrono>
#include <cmath>
#include <string>

#ifdef INTERACTION_HAS_UI_TESTING
#include <huxerui/testing/ui_test.h>
using namespace huxerui::testing;

using namespace huxerui;
using namespace huxerui::interaction;

namespace {

View PinchAndSwipeGallery() {
  auto deleted = UseState(0);
  auto dismissed = UseState(0);
  auto width = UseState(100.0F);
  return Column {
    PinchZoom(
        Text("zoom target")
            .With(Frame{.width = 100.0F, .height = 50.0F}, Background{Color::Rgb(90, 130, 90)})
            .Key("zoom-content"),
        {.minimum_scale = 1.0F, .maximum_scale = 3.0F, .spring_back = false})
        .Key("pinch"),
    SwipeActions(
        Text("swipe row")
            .With(Frame{.width = 300.0F, .height = 56.0F}, Background{Color::Rgb(80, 110, 160)})
            .Key("swipe-content"),
        [deleted](SwipeActionContext ctx) {
          return Button("Delete")
              .OnClick([deleted, close = ctx.close] {
                deleted += 1;
                close();
              })
              .Key("delete");
        },
        {.action_extent = 96.0F, .activation_ratio = 0.4F})
        .Key("swipe"),
    Dismissible(
        Text("dismiss target")
            .With(Frame{.width = 200.0F, .height = 48.0F}, Background{Color::Rgb(80, 120, 160)}),
        [](float progress, float) {
          return Text(progress > 0.5F ? "release" : "drag").With(Background{Color::Rgb(180, 60, 60)});
        },
        [dismissed] { dismissed += 1; },
        {.dismiss_threshold = 80.0F, .minimum_velocity = 10000.0F, .exit_distance = 240.0F, .exit_duration = 0.15})
        .Key("dismiss"),
    Row {
      Text("panel").With(Frame{.width = width.Get(), .height = 48.0F}, Background{Color::Rgb(80, 110, 160)}),
      ResizeHandle(Axis::Horizontal, [width](float delta) {
        width = std::clamp(width.Get() + delta, 60.0F, 240.0F);
      }).Key("resize-handle"),
    }.With(CrossAlign(CrossAxisAlignment::Stretch)),
    Text("deleted: " + std::to_string(deleted.Get())),
    Text("dismissed: " + std::to_string(dismissed.Get())),
    Text("width: " + std::to_string(static_cast<int>(std::lround(width.Get())))),
  }.With(Padding(16.0F), Spacing(12.0F));
}

View TransformAndInertiaGallery() {
  auto started = UseState(0);
  auto ended = UseState(0);
  auto last = UseState(0);
  return Column {
    PanZoom(
        Text("transform target")
            .With(Frame{.width = 160.0F, .height = 80.0F}, Background{Color::Rgb(80, 120, 160)})
            .Key("transform-content"),
        {.minimum_scale = 0.5F, .maximum_scale = 3.0F})
        .Key("panzoom"),
    InertialDrag(
        Text("inertial target")
            .With(Frame{.width = 160.0F, .height = 50.0F}, Background{Color::Rgb(150, 100, 70)})
            .Key("inertial-content"),
        {.projection_time = 0.2F, .maximum_projection = 300.0F})
        .Key("inertial"),
    PressDragHandle(
        Text("handle").With(Frame{.width = 160.0F, .height = 48.0F}, Background{Color::Rgb(120, 90, 150)}),
        {.on_started = [started](const DragEvent&) { started += 1; },
         .on_changed = [last](const DragEvent& e) { last = static_cast<int>(e.translation.x); },
         .on_ended = [ended](const DragEvent&) { ended += 1; }},
        {.press_duration = std::chrono::duration<double>(0.2)})
        .Key("press-drag"),
    Text("started: " + std::to_string(started.Get())),
    Text("ended: " + std::to_string(ended.Get())),
    Text("last: " + std::to_string(last.Get())),
  }.With(Padding(16.0F), Spacing(12.0F));
}

} // namespace

void CheckGestureContracts() {
  {
    Application app(PinchAndSwipeGallery, {.window = {.initial_size = {520, 520}}, .show_debug_overlay = false});
    UiTest ui(app, {.viewport = {520, 520}});

    // PinchZoom wheel zoom clamping.
    const Rect initial_zoom = ui.Find(UiSelector::Key("zoom-content")).One().bounds;
    const Point zoom_center = Center(initial_zoom);
    const Point plain = ui.SendScroll({.position = zoom_center, .delta_y = 120.0F});
    ui.PumpAndSettle();
    CHECK(plain.y == 0.0F);
    CHECK(Near(ui.Find(UiSelector::Key("zoom-content")).One().bounds.width, initial_zoom.width));

    const Point consumed = ui.SendScroll(
        {.position = zoom_center, .delta_y = -1000.0F, .modifiers = KeyModifiers{.control = true}});
    ui.PumpAndSettle();
    CHECK(consumed.y != 0.0F);
    CHECK(Near(ui.Find(UiSelector::Key("zoom-content")).One().bounds.width, initial_zoom.width * 3.0F));

    ui.SendScroll({.position = zoom_center, .delta_y = 1000.0F, .modifiers = KeyModifiers{.control = true}});
    ui.PumpAndSettle();
    CHECK(Near(ui.Find(UiSelector::Key("zoom-content")).One().bounds.width, initial_zoom.width));

    // SwipeActions reveal and close via SwipeActionContext.
    const Rect swipe_initial = ui.Find(UiSelector::Key("swipe-content")).One().bounds;
    const Point swipe_start = Center(swipe_initial);
    ui.Drag(swipe_start, {swipe_start.x - 120.0F, swipe_start.y},
            {.duration = std::chrono::duration<double>(0.3), .segments = 12});
    ui.PumpAndSettle();
    CHECK(ui.Find(UiSelector::Key("swipe-content")).One().bounds.x < swipe_initial.x - 40.0F);
    ui.Find(UiSelector::Key("delete")).Tap();
    ui.PumpAndSettle();
    CHECK(ui.Find(UiSelector::Text("deleted: 1")).Exists());
    CHECK(Near(ui.Find(UiSelector::Key("swipe-content")).One().bounds.x, swipe_initial.x));

    // Dismissible with custom background builder.
    const Rect dismiss_rect = ui.Find(UiSelector::Key("dismiss")).One().bounds;
    const Point dismiss_start{dismiss_rect.x + 40.0F, dismiss_rect.y + dismiss_rect.height * 0.5F};
    ui.Drag(dismiss_start, {dismiss_start.x + 120.0F, dismiss_start.y},
            {.duration = std::chrono::duration<double>(0.2), .segments = 8});
    ui.PumpAndSettle();
    CHECK(ui.Find(UiSelector::Text("dismissed: 1")).Exists());

    // ResizeHandle delta forwarding.
    const Point rh = Center(ui.Find(UiSelector::Key("resize-handle")).One().bounds);
    ui.Drag(rh, {rh.x + 40.0F, rh.y}, {.duration = std::chrono::duration<double>(0.2), .segments = 8});
    ui.PumpAndSettle();
    CHECK(ui.Find(UiSelector::Text("width: 140")).Exists());
  }

  {
    Application app(TransformAndInertiaGallery, {.window = {.initial_size = {520, 520}}, .show_debug_overlay = false});
    UiTest ui(app, {.viewport = {520, 520}});

    // Two-pointer PanZoom scale + pan + rotation.
    const Rect initial = ui.Find(UiSelector::Key("transform-content")).One().bounds;
    const Point center = Center(initial);
    const Point left{center.x - 30.0F, center.y};
    const Point right{center.x + 30.0F, center.y};
    PointerEvent first{.type = PointerEventType::Down,
                       .pointer_id = 21,
                       .position = left,
                       .device_kind = PointerDeviceKind::Touch,
                       .changed_button = PointerButton::Primary,
                       .pressed_buttons = PointerButton::Primary};
    PointerEvent second = first;
    second.pointer_id = 22;
    second.position = right;
    ui.SendPointer(first);
    ui.SendPointer(second);
    ui.Pump();
    first.type = PointerEventType::Move;
    first.position = {center.x - 60.0F, center.y};
    second.type = PointerEventType::Move;
    second.position = {center.x + 60.0F, center.y};
    ui.SendPointer(first);
    ui.SendPointer(second);
    ui.PumpAndSettle();
    const Rect scaled = ui.Find(UiSelector::Key("transform-content")).One().bounds;
    CHECK(scaled.width > initial.width * 1.2F);
    first.type = PointerEventType::Up;
    first.pressed_buttons = PointerButton::None;
    second.type = PointerEventType::Up;
    second.pressed_buttons = PointerButton::None;
    ui.SendPointer(first);
    ui.SendPointer(second);
    ui.PumpAndSettle();

    // InertialDrag velocity projection.
    const Rect inertia_initial = ui.Find(UiSelector::Key("inertial-content")).One().bounds;
    const Point inertia_start = Center(inertia_initial);
    ui.Drag(inertia_start, {inertia_start.x + 60.0F, inertia_start.y},
            {.duration = std::chrono::duration<double>(0.1), .segments = 5});
    ui.PumpAndSettle();
    CHECK(ui.Find(UiSelector::Key("inertial-content")).One().bounds.x > inertia_initial.x + 60.0F);

    // PressDragHandle hold-then-drag.
    const Point pd_start = Center(ui.Find(UiSelector::Key("press-drag")).One().bounds);
    PointerEvent hold{.type = PointerEventType::Down,
                      .pointer_id = 61,
                      .position = pd_start,
                      .device_kind = PointerDeviceKind::Touch,
                      .changed_button = PointerButton::Primary,
                      .pressed_buttons = PointerButton::Primary};
    ui.SendPointer(hold);
    ui.Pump(std::chrono::milliseconds(250));
    hold.type = PointerEventType::Move;
    hold.position = {pd_start.x + 40.0F, pd_start.y};
    ui.SendPointer(hold);
    ui.Pump();
    hold.type = PointerEventType::Up;
    hold.pressed_buttons = PointerButton::None;
    ui.SendPointer(hold);
    ui.PumpAndSettle();
    CHECK(ui.Find(UiSelector::Text("started: 1")).Exists());
    CHECK(ui.Find(UiSelector::Text("ended: 1")).Exists());
    CHECK(ui.Find(UiSelector::Text("last: 40")).Exists());
  }
}

#endif
