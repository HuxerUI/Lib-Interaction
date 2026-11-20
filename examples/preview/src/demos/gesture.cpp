#include "demos.h"

#include <string>
#include <utility>
#include <vector>

namespace interaction_demo {
using namespace huxerui;
using namespace huxerui::interaction;

View GestureSection(const DemoState& state, const DemoContext& context) {
  return Scope([state, context]() -> View {
    auto burst = UseState(FireworksBurst{});
    auto pop = UsePop();
    const auto card_label = Foreground(context.colors.on_primary_container);
    const auto card = Frame{.width = 240.0F, .height = 96.0F};

    return Column {
      SectionLabel(context, "Gesture"),
      Flow {
        LongPressable(
            Stack { Text("Hold me", TextRole::Label).With(card_label) }.With(
                card,
                Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                Background{context.colors.primary_container}, CornerRadius(14.0F)),
            [holds = state.holds] { holds += 1; },
            {.show_progress = true})
            .Key("longpress-demo"),
        DoubleClickable(
            Stack { Text("Double-click me", TextRole::Label).With(card_label) }.With(
                card,
                Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                Background{context.colors.primary_container}, CornerRadius(14.0F)),
            [doubles = state.doubles, burst](const MultiTapEvent& event) {
              doubles += 1;
              burst = FireworksBurst{.id = burst.Get().id + 1, .local_position = event.position};
            })
            .With(Fireworks{FireworksStyle{.corner_radii = 14.0F}, burst.Get()})
            .Key("doubleclick-demo"),
        Popped(
            Stack { Text("Pop", TextRole::Label).With(Foreground(context.colors.on_primary)) }.With(
                Frame{.width = 96.0F, .height = 40.0F},
                Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                Background{context.colors.primary}, CornerRadius(20.0F)),
            pop)
            .OnClick([pop] { pop.Trigger(); })
            .With(PointerCursor{PointerCursorKind::Hand})
            .Key("pop-demo"),
        Stack { Text("Loading…", TextRole::Label).With(card_label) }.With(
                card,
                Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                Background{context.colors.primary_container}, CornerRadius(14.0F),
                Shimmer{ShimmerStyle{.corner_radii = 14.0F}})
            .Key("shimmer-demo"),
      }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Center)),
      Scope([context]() -> View {
        auto deck = UseSwipeDeckController();
        auto last_swipe = UseState(std::string{"drag a card or tap an action"});
        struct CardItem {
          const char* title;
          const char* subtitle;
        };
        static constexpr CardItem kItems[] = {
            {"Alpha Card", "Swipe right to keep, left to skip"},
            {"Velocity Flick", "Quick horizontal flick commits exit"},
            {"Depth Stack", "Back cards scale and rise smoothly"},
            {"Spring Return", "Short drags snap back to center"},
            {"Rewind History", "Use Rewind to pull cards back in"},
        };

        return Column {
          Text("SwipeDeck (stacked card deck): " + std::to_string(deck.RemainingCount()) + " left · " +
                   last_swipe.Get(),
               TextRole::Label)
              .With(Foreground(context.colors.on_surface_variant)),
          SwipeDeck(
              std::size(kItems),
              [context](SwipeDeckCardContext ctx) -> View {
                const bool is_right = ctx.direction == SwipeDirection::Right;
                const bool is_left = ctx.direction == SwipeDirection::Left;
                const float badge_alpha = ctx.is_top ? std::clamp(ctx.progress * 1.3F, 0.0F, 1.0F) : 0.0F;
                return Column {
                  Row {
                    Text(is_right ? "LIKE" : (is_left ? "NOPE" : "CARD"), TextRole::Label)
                        .With(Foreground(is_left ? context.colors.error : context.colors.primary),
                              Opacity(ctx.is_top && (is_right || is_left) ? badge_alpha : 0.65F)),
                    Spacer{},
                    Text("#" + std::to_string(ctx.index + 1), TextRole::Label)
                        .With(Foreground(context.colors.on_surface_variant)),
                  }.With(CrossAlign(CrossAxisAlignment::Center)),
                  Spacer{},
                  Text(kItems[ctx.index].title, TextRole::Title)
                      .With(Foreground(context.colors.on_surface)),
                  Text(kItems[ctx.index].subtitle, TextRole::Body)
                      .With(Foreground(context.colors.on_surface_variant)),
                }
                    .With(Frame{.width = 248.0F, .height = 136.0F}, Padding(14.0F), Spacing(4.0F),
                          Background{context.colors.surface_container_high},
                          Border{.color = context.colors.outline, .width = 1.0F}, CornerRadius(14.0F),
                          Shadow{.color = Color::Rgb(0, 0, 0, 0.14F), .offset = {0.0F, 4.0F}, .blur_radius = 12.0F});
              },
              [last_swipe](std::size_t idx, SwipeDirection dir) {
                last_swipe = "card #" + std::to_string(idx + 1)
                             + (dir == SwipeDirection::Right ? " → liked" : " ← skipped");
              },
              {.controller = deck},
              [context, deck]() -> View {
                return Stack {
                  Column {
                    Text("Deck empty", TextRole::Label).With(Foreground(context.colors.on_surface_variant)),
                    Button("Reset Deck").OnClick([deck] { deck.Reset(); }),
                  }.With(Spacing(8.0F), CrossAlign(CrossAxisAlignment::Center)),
                }.With(Frame{.width = 248.0F, .height = 136.0F},
                       Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                       Background{context.colors.surface_container_low}, CornerRadius(14.0F));
              }),
          Row {
            Button("Nope").OnClick([deck] { deck.Swipe(SwipeDirection::Left); }),
            Button("Rewind")
                .With(Enabled{deck.CanRewind()})
                .OnClick([deck] { deck.Rewind(); }),
            Button("Like").OnClick([deck] { deck.Swipe(SwipeDirection::Right); }),
          }.With(Spacing(10.0F)),
        }.With(Spacing(10.0F), CrossAlign(CrossAxisAlignment::Start));
      }).Key("swipedeck-demo"),
      Scope([context]() -> View {
        auto compare = UseCompareSliderController(0.5F);
        auto vertical_axis = UseState(false);
        const Axis axis = vertical_axis.Get() ? Axis::Vertical : Axis::Horizontal;
        const int pct = static_cast<int>(compare.Progress() * 100.0F + 0.5F);

        View before_card = Stack {
          Column {
            Text("BEFORE", TextRole::Label).With(Foreground(context.colors.on_primary)),
            Text("Wireframe / Original", TextRole::Title).With(Foreground(context.colors.on_primary)),
            Text("Full-bleed curtain layer clipped without squishing", TextRole::Body)
                .With(Foreground(context.colors.on_primary)),
          }.With(Spacing(4.0F), CrossAlign(CrossAxisAlignment::Start)),
        }.With(Frame{.width = 300.0F, .height = 148.0F}, Padding(14.0F),
               Align(HorizontalAlignment::Start, VerticalAlignment::Center), Background{context.colors.primary});

        View after_card = Stack {
          Column {
            Text("AFTER", TextRole::Label).With(Foreground(context.colors.on_surface_variant)),
            Text("Rendered / Graded", TextRole::Title).With(Foreground(context.colors.on_surface)),
            Text("Drag divider, double-click to reset, or use Arrow keys", TextRole::Body)
                .With(Foreground(context.colors.on_surface_variant)),
          }.With(Spacing(4.0F), CrossAlign(CrossAxisAlignment::End)),
        }.With(Frame{.width = 300.0F, .height = 148.0F}, Padding(14.0F),
               Align(HorizontalAlignment::End, VerticalAlignment::Center),
               Background{context.colors.surface_container_high});

        return Column {
          Text("CompareSlider", TextRole::Title),
          Row {
            Text(std::to_string(pct) + "% split · " + (axis == Axis::Horizontal ? "Horizontal" : "Vertical"),
                 TextRole::Label)
                .With(Foreground(context.colors.on_surface_variant)),
            Spacer{},
            Button(axis == Axis::Horizontal ? "Axis: H" : "Axis: V")
                .OnClick([vertical_axis] { vertical_axis = !vertical_axis.Get(); }),
            Button("Reset").OnClick([compare] { compare.Reset(); }),
          }.With(Frame{.width = 300.0F}, Spacing(8.0F), CrossAlign(CrossAxisAlignment::Center)),
          CompareSlider(std::move(before_card), std::move(after_card),
                        CompareSliderStyle{
                            .axis = axis,
                            .handle_accent_color = context.colors.primary,
                            .controller = compare,
                        })
              .With(Frame{.width = 300.0F, .height = 148.0F}, CornerRadius(12.0F),
                    Border{.color = context.colors.outline, .width = 1.0F}),
        }.With(Spacing(10.0F), CrossAlign(CrossAxisAlignment::Start));
      }).Key("compareslider-demo"),
      Scope([context]() -> View {
        auto x_ctrl = UseScrubValueController(0.0);
        auto op_ctrl = UseScrubValueController(100.0);
        auto scale_ctrl = UseScrubValueController(1.0);

        auto pill = [context](const std::string& title, const std::string& val_text,
                              const std::string& hint, bool active, ScrubPrecision tier) -> View {
          const Color border = active ? context.colors.primary : context.colors.outline;
          const std::string tier_label =
              tier == ScrubPrecision::Fine ? " [Fine 0.1x]"
                                           : (tier == ScrubPrecision::Coarse ? " [Coarse 10x]" : "");
          return Column {
            Row {
              Text(title, TextRole::Label).With(Foreground(context.colors.on_surface_variant)),
              Spacer{},
              Text(val_text + tier_label, TextRole::Label)
                  .With(Foreground(active ? context.colors.primary : context.colors.on_surface)),
            }.With(CrossAlign(CrossAxisAlignment::Center)),
            Text(hint, TextRole::Body).With(Foreground(context.colors.on_surface_variant)),
          }
              .With(Frame{.width = 300.0F}, Padding(EdgeInsets::Symmetric(12.0F, 8.0F)), Spacing(2.0F),
                    Background{context.colors.surface_container_low},
                    Border{.color = border, .width = active ? 2.0F : 1.0F}, CornerRadius(10.0F));
        };

        return Column {
          Row {
            Text("ScrubValue", TextRole::Title),
            Spacer{},
            Button("Reset All").OnClick([x_ctrl, op_ctrl, scale_ctrl] {
              x_ctrl.Reset();
              op_ctrl.Reset();
              scale_ctrl.Reset();
            }),
          }.With(Frame{.width = 300.0F}, CrossAlign(CrossAxisAlignment::Center)),
          ScrubValue(
              [&](ScrubValueSnapshot s) -> View {
                const int iv = static_cast<int>(std::lround(s.value));
                return pill("X Offset (H)", std::to_string(iv) + " px",
                            "Drag horizontally · Shift fine · Alt coarse · Dbl-click reset",
                            s.scrubbing || s.focused, s.precision);
              },
              {
                  .initial_value = 0.0,
                  .min_value = -200.0,
                  .max_value = 200.0,
                  .step = 1.0,
                  .controller = x_ctrl,
              }),
          ScrubValue(
              [&](ScrubValueSnapshot s) -> View {
                const int iv = static_cast<int>(std::lround(s.value));
                return pill("Opacity (H)", std::to_string(iv) + "%",
                            "Clamped [0, 100], step 1", s.scrubbing || s.focused, s.precision);
              },
              {
                  .initial_value = 100.0,
                  .min_value = 0.0,
                  .max_value = 100.0,
                  .step = 1.0,
                  .controller = op_ctrl,
              }),
          ScrubValue(
              [&](ScrubValueSnapshot s) -> View {
                const int pct = static_cast<int>(std::lround(s.value * 100.0));
                return pill("Scale (V)", std::to_string(pct) + "%",
                            "Drag vertically · step 0.05", s.scrubbing || s.focused, s.precision);
              },
              {
                  .initial_value = 1.0,
                  .min_value = 0.25,
                  .max_value = 4.0,
                  .sensitivity = 0.02,
                  .step = 0.05,
                  .axis = Axis::Vertical,
                  .controller = scale_ctrl,
              }),
        }.With(Spacing(8.0F), CrossAlign(CrossAxisAlignment::Start));
      }).Key("scrubvalue-demo"),
      Scope([context]() -> View {
        auto gain_dial = UseRadialDialController(35.0);
        auto spin_dial = UseRadialDialController(0.0);
        const int gain_val = static_cast<int>(std::lround(gain_dial.Value()));
        const int spin_deg = static_cast<int>(std::lround(spin_dial.Value()));
        const int spin_turns_x10 = static_cast<int>(std::lround((spin_dial.Value() / 360.0) * 10.0));
        return Column {
          Row {
            Text("RadialDial", TextRole::Title),
            Spacer{},
            Button("Reset").OnClick([gain_dial, spin_dial] {
              gain_dial.Reset();
              spin_dial.Reset();
            }),
          }.With(Frame{.width = 300.0F}, CrossAlign(CrossAxisAlignment::Center)),
          Row {
            Column {
              RadialDial({
                  .mode = RadialDialMode::Bounded,
                  .initial_value = 35.0,
                  .min_value = 0.0,
                  .max_value = 100.0,
                  .track_color = context.colors.surface_container_highest,
                  .active_color = context.colors.primary,
                  .knob_color = context.colors.surface_container_low,
                  .controller = gain_dial,
              }),
              Text("Gain: " + std::to_string(gain_val) + "%", TextRole::Label)
                  .With(Foreground(context.colors.on_surface)),
              Text("Bounded 270° · Wheel / Drag", TextRole::Body)
                  .With(Foreground(context.colors.on_surface_variant)),
            }.With(Spacing(4.0F), CrossAlign(CrossAxisAlignment::Center)),
            Column {
              RadialDial({
                  .mode = RadialDialMode::Continuous,
                  .initial_value = 0.0,
                  .min_value = -std::numeric_limits<double>::infinity(),
                  .max_value = std::numeric_limits<double>::infinity(),
                  .start_angle_degrees = -90.0,
                  .value_per_turn = 360.0,
                  .track_color = context.colors.surface_container_highest,
                  .active_color = context.colors.secondary,
                  .knob_color = context.colors.surface_container_low,
                  .controller = spin_dial,
              }),
              Text("Angle: " + std::to_string(spin_deg) + "° (" + std::to_string(spin_turns_x10 / 10) + "."
                   + std::to_string(std::abs(spin_turns_x10 % 10)) + "t)",
                   TextRole::Label)
                  .With(Foreground(context.colors.on_surface)),
              Text("Continuous Multi-Turn", TextRole::Body)
                  .With(Foreground(context.colors.on_surface_variant)),
            }.With(Spacing(4.0F), CrossAlign(CrossAxisAlignment::Center)),
          }.With(Frame{.width = 300.0F}, MainAlign(MainAxisAlignment::SpaceEvenly),
                 CrossAlign(CrossAxisAlignment::Start)),
        }.With(Spacing(10.0F), CrossAlign(CrossAxisAlignment::Start));
      }).Key("radialdial-demo"),
    }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Stretch));
  });
}

} // namespace interaction_demo
