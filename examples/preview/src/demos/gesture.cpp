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
    }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Stretch));
  });
}

} // namespace interaction_demo
