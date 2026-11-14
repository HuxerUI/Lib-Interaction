#include "demos.h"

#include <huxerui/vector.h>

#include <utility>

namespace interaction_demo {
using namespace huxerui;
using namespace huxerui::interaction;

namespace {

/// Appends a closed elliptic contour to a path; multiple contours fill as a union under NonZero.
void AppendEllipse(Path& path, Point center, Size radii) {
  constexpr float kCircle = 0.5522847498F;
  const float horizontal = radii.width * kCircle;
  const float vertical = radii.height * kCircle;
  path.MoveTo({center.x, center.y - radii.height});
  path.CubicTo({center.x + horizontal, center.y - radii.height}, {center.x + radii.width, center.y - vertical},
               {center.x + radii.width, center.y});
  path.CubicTo({center.x + radii.width, center.y + vertical}, {center.x + horizontal, center.y + radii.height},
               {center.x, center.y + radii.height});
  path.CubicTo({center.x - horizontal, center.y + radii.height}, {center.x - radii.width, center.y + vertical},
               {center.x - radii.width, center.y});
  path.CubicTo({center.x - radii.width, center.y - vertical}, {center.x - horizontal, center.y - radii.height},
               {center.x, center.y - radii.height});
  path.Close();
}

/// Teardrop toe bean shared by the silhouette and jelly assets.
Path ToeBeanPath() {
  Path bean;
  bean.MoveTo({50.0F, 4.0F});
  bean.CubicTo({76.0F, 2.0F}, {94.0F, 18.0F}, {96.0F, 42.0F});
  bean.CubicTo({98.0F, 68.0F}, {78.0F, 96.0F}, {50.0F, 96.0F});
  bean.CubicTo({22.0F, 96.0F}, {2.0F, 68.0F}, {4.0F, 42.0F});
  bean.CubicTo({6.0F, 18.0F}, {24.0F, 2.0F}, {50.0F, 4.0F});
  bean.Close();
  return bean;
}

/// Wide metacarpal pad with a soft notch between the two upper lobes.
Path PalmPadPath() {
  Path pad;
  pad.MoveTo({12.0F, 24.0F});
  pad.CubicTo({20.0F, 12.0F}, {34.0F, 12.0F}, {42.0F, 18.0F});
  pad.CubicTo({46.0F, 22.0F}, {54.0F, 22.0F}, {58.0F, 18.0F});
  pad.CubicTo({66.0F, 12.0F}, {80.0F, 12.0F}, {88.0F, 24.0F});
  pad.CubicTo({98.0F, 38.0F}, {98.0F, 60.0F}, {86.0F, 74.0F});
  pad.CubicTo({77.0F, 85.0F}, {65.0F, 92.0F}, {50.0F, 92.0F});
  pad.CubicTo({35.0F, 92.0F}, {23.0F, 85.0F}, {14.0F, 74.0F});
  pad.CubicTo({2.0F, 60.0F}, {2.0F, 38.0F}, {12.0F, 24.0F});
  pad.Close();
  return pad;
}

/// Plain bean silhouette used for tinted wells and the hover gloss.
const VectorAsset& ToeBeanAsset() {
  static const VectorAsset asset = VectorAsset::Create({100.0F, 100.0F}, [](VectorBuilder& builder) {
    builder.FillPath(ToeBeanPath(), Color::White());
  });
  return asset;
}

/// Plain palm silhouette used for tinted wells and the hover gloss.
const VectorAsset& PalmPadAsset() {
  static const VectorAsset asset = VectorAsset::Create({100.0F, 100.0F}, [](VectorBuilder& builder) {
    builder.FillPath(PalmPadPath(), Color::White());
  });
  return asset;
}

/// Soft bubblegum-pink jelly shading, light from the upper left.
Brush JellyBrush() {
  return RadialGradient{
      .center = {0.36F, 0.28F},
      .radius = {0.72F, 0.68F},
      .stops = {{0.0F, Color::Rgb(255, 220, 230)},
                {0.46F, Color::Rgb(255, 178, 198)},
                {0.82F, Color::Rgb(249, 143, 169)},
                {1.0F, Color::Rgb(239, 120, 150)}},
  };
}

/// Glossy toe bean with a baked radial gradient, specular swoosh, and a small reflection.
const VectorAsset& ToeBeanJellyAsset() {
  static const VectorAsset asset = VectorAsset::Create({100.0F, 100.0F}, [](VectorBuilder& builder) {
    const Path bean = ToeBeanPath();
    builder.FillPath(bean, JellyBrush());
    builder.StrokePath(bean, Color::Rgb(241, 128, 156, 0.45F),
                       StrokeStyle{.width = 1.2F, .join = StrokeJoin::Round});
    builder.PushClip(bean);
    Path gloss;
    gloss.MoveTo({20.0F, 34.0F});
    gloss.CubicTo({24.0F, 16.0F}, {42.0F, 10.0F}, {56.0F, 21.0F});
    gloss.CubicTo({42.0F, 19.0F}, {29.0F, 25.0F}, {20.0F, 34.0F});
    gloss.Close();
    builder.FillPath(gloss, Color::Rgb(255, 255, 255, 0.90F));
    Path dot;
    AppendEllipse(dot, {29.0F, 46.0F}, {5.4F, 3.0F});
    builder.FillPath(dot, Color::Rgb(255, 255, 255, 0.70F));
    Path sheen;
    AppendEllipse(sheen, {72.0F, 74.0F}, {9.0F, 4.0F});
    builder.FillPath(sheen, Color::Rgb(255, 255, 255, 0.15F));
    builder.PopClip();
  });
  return asset;
}

/// Glossy palm pad with a wider specular swoosh and a lower-right reflection.
const VectorAsset& PalmPadJellyAsset() {
  static const VectorAsset asset = VectorAsset::Create({100.0F, 100.0F}, [](VectorBuilder& builder) {
    const Path pad = PalmPadPath();
    builder.FillPath(pad, JellyBrush());
    builder.StrokePath(pad, Color::Rgb(241, 128, 156, 0.45F),
                       StrokeStyle{.width = 1.2F, .join = StrokeJoin::Round});
    builder.PushClip(pad);
    Path gloss;
    gloss.MoveTo({16.0F, 38.0F});
    gloss.CubicTo({20.0F, 18.0F}, {48.0F, 10.0F}, {66.0F, 26.0F});
    gloss.CubicTo({50.0F, 21.0F}, {31.0F, 26.0F}, {16.0F, 38.0F});
    gloss.Close();
    builder.FillPath(gloss, Color::Rgb(255, 255, 255, 0.88F));
    Path dot;
    AppendEllipse(dot, {30.0F, 52.0F}, {6.0F, 3.4F});
    builder.FillPath(dot, Color::Rgb(255, 255, 255, 0.64F));
    Path sheen;
    AppendEllipse(sheen, {76.0F, 78.0F}, {11.0F, 5.0F});
    builder.FillPath(sheen, Color::Rgb(255, 255, 255, 0.14F));
    builder.PopClip();
  });
  return asset;
}

/// Builds the fluffy paw cushion silhouette in stage coordinates.
Path PawBaseUnion(Point shift, float grow) {
  Path path;
  AppendEllipse(path, {120.0F + shift.x, 141.0F + shift.y}, {112.0F + grow, 85.0F + grow});
  AppendEllipse(path, {120.0F + shift.x, 72.0F + shift.y}, {30.0F + grow, 38.0F + grow});
  AppendEllipse(path, {36.0F + shift.x, 104.0F + shift.y}, {32.0F + grow, 38.0F + grow});
  AppendEllipse(path, {204.0F + shift.x, 104.0F + shift.y}, {32.0F + grow, 38.0F + grow});
  AppendEllipse(path, {90.0F + shift.x, 66.0F + shift.y}, {44.0F + grow, 44.0F + grow});
  AppendEllipse(path, {150.0F + shift.x, 66.0F + shift.y}, {44.0F + grow, 44.0F + grow});
  return path;
}

/// Cream cushion behind the pads, with a baked pink under-rim.
const VectorAsset& PawBaseAsset() {
  static const VectorAsset asset = VectorAsset::Create({240.0F, 230.0F}, [](VectorBuilder& builder) {
    builder.FillPath(PawBaseUnion({0.0F, 2.0F}, 1.0F), Color::Rgb(255, 223, 233));
    builder.FillPath(PawBaseUnion({0.0F, 0.0F}, 0.0F),
                     LinearGradient{
                         .start = {0.22F, 0.0F},
                         .end = {0.78F, 1.0F},
                         .stops = {{0.0F, Color::White()},
                                   {0.58F, Color::Rgb(255, 251, 253)},
                                   {1.0F, Color::Rgb(255, 232, 240)}},
                     });
  });
  return asset;
}

/// Four-point sparkle that twinkles while the paw is hovered.
const VectorAsset& SparkleAsset() {
  static const VectorAsset asset = VectorAsset::Create({100.0F, 100.0F}, [](VectorBuilder& builder) {
    Path star;
    star.MoveTo({50.0F, 0.0F});
    star.LineTo({58.0F, 42.0F});
    star.LineTo({100.0F, 50.0F});
    star.LineTo({58.0F, 58.0F});
    star.LineTo({50.0F, 100.0F});
    star.LineTo({42.0F, 58.0F});
    star.LineTo({0.0F, 50.0F});
    star.LineTo({42.0F, 42.0F});
    star.Close();
    builder.FillPath(std::move(star), Color::White());
  });
  return asset;
}

} // namespace

View BloomSection(const DemoState& state, const DemoContext& context) {
  const bool paw_blooming = state.paw_hovered.Get();
  const Color paw_well = context.dark ? Color::Rgb(211, 123, 150) : Color::Rgb(237, 149, 173);
  const Color paw_base_shade = context.dark ? Color::Rgb(198, 111, 139) : Color::Rgb(239, 147, 174);
  const Color paw_glow = context.dark ? Color::Rgb(255, 154, 186, 0.44F) : Color::Rgb(255, 146, 177, 0.36F);
  const Color paw_glow_soft = context.dark ? Color::Rgb(255, 154, 186, 0.16F)
                                           : Color::Rgb(255, 146, 177, 0.12F);
  const Color paw_ink = context.dark ? Color::Rgb(255, 205, 219) : Color::Rgb(141, 63, 85);
  const Color paw_muted = context.dark ? Color::Rgb(213, 164, 180) : Color::Rgb(176, 108, 128);
  const Color paw_card_top = context.dark ? Color::Rgb(52, 38, 46) : Color::Rgb(255, 249, 251);
  const Color paw_card_bottom = context.dark ? Color::Rgb(42, 31, 38) : Color::Rgb(255, 236, 242);
  const Color paw_border = context.dark ? Color::Rgb(255, 176, 199, 0.16F) : Color::Rgb(232, 126, 157, 0.34F);
  const Color paw_shadow = context.dark ? Color::Rgb(0, 0, 0, 0.34F) : Color::Rgb(190, 92, 124, 0.18F);
  const SpringSpec bloom_motion{.stiffness = 300.0F, .damping_ratio = 0.55F};
  const TweenSpec fade{.duration = 0.24, .easing = Easing::EaseOut};
  const TweenSpec sparkle_motion{.duration = 0.32, .easing = Easing::EaseOut};

  const auto pad = [&](const VectorAsset& silhouette, const VectorAsset& jelly, Size size, Point rest, Point drift,
                       float rest_angle, float hover_angle, float hover_scale) -> View {
    return Stack {
      Image(silhouette).Tint(paw_well).With(Frame{.width = size.width, .height = size.height},
                                            Scale(1.14F),
                                            Offset(Point{0.0F, 4.0F}),
                                            Opacity(0.26F)),
      Image(jelly).With(Frame{.width = size.width, .height = size.height}),
      Image(silhouette).Tint(Color::White()).With(
          Frame{.width = size.width, .height = size.height},
          Opacity(AnimateTo(paw_blooming ? 0.16F : 0.0F, fade))),
    }.With(Frame{.width = size.width, .height = size.height},
           Align(HorizontalAlignment::Center, VerticalAlignment::Center),
           Rotation(AnimateTo(paw_blooming ? hover_angle : rest_angle, bloom_motion)),
           Scale(AnimateTo(paw_blooming ? hover_scale : 1.0F, bloom_motion)),
           Offset(AnimateTo(paw_blooming ? rest + drift : rest, bloom_motion)));
  };
  const auto sparkle = [&](Point position, float size, float rest_angle, float hover_angle,
                           double delay) -> View {
    return Image(SparkleAsset())
        .Tint(Color::White())
        .With(Frame{.width = size, .height = size},
              Rotation(AnimateTo(paw_blooming ? hover_angle : rest_angle, sparkle_motion,
                                 AnimationPlayback{.delay = paw_blooming ? delay : 0.0})),
              Scale(AnimateTo(paw_blooming ? 1.0F : 0.35F, sparkle_motion)),
              Opacity(AnimateTo(paw_blooming ? 0.92F : 0.0F, sparkle_motion)),
              Offset(position));
  };
  View halo = Stack{}.With(
      Frame{.width = 232.0F, .height = 232.0F},
      Background{RadialGradient{
          .center = {0.5F, 0.5F},
          .radius = {0.5F, 0.5F},
          .stops = {{0.0F, paw_glow}, {0.58F, paw_glow_soft}, {1.0F, Color::Rgb(255, 170, 195, 0.0F)}},
      }},
      Scale(AnimateTo(paw_blooming ? 1.0F : 0.72F, bloom_motion)),
      Opacity(AnimateTo(paw_blooming ? 1.0F : 0.0F, fade)),
      Offset(Point{4.0F, 6.0F}));
  View base_shadow = Image(PawBaseAsset())
                         .Tint(paw_base_shade)
                         .With(Frame{.width = 240.0F, .height = 230.0F},
                               Scale(1.015F),
                               Offset(Point{0.0F, 4.0F}),
                               Opacity(0.14F));
  View base = Image(PawBaseAsset()).With(Frame{.width = 240.0F, .height = 230.0F});
  View stage = Stack {
    halo,
    base_shadow,
    base,
    pad(PalmPadAsset(), PalmPadJellyAsset(), {122.0F, 90.0F}, {59.0F, 124.0F}, {0.0F, 2.0F}, 0.0F, 0.0F, 1.04F),
    pad(ToeBeanAsset(), ToeBeanJellyAsset(), {52.0F, 68.0F}, {8.0F, 76.0F}, {-10.0F, -6.0F}, -26.0F, -35.0F, 1.07F),
    pad(ToeBeanAsset(), ToeBeanJellyAsset(), {56.0F, 72.0F}, {62.0F, 33.0F}, {-5.0F, -14.0F}, -10.0F, -15.0F, 1.07F)
        .Key("catpaw-toe"),
    pad(ToeBeanAsset(), ToeBeanJellyAsset(), {56.0F, 72.0F}, {122.0F, 33.0F}, {5.0F, -14.0F}, 10.0F, 15.0F, 1.07F),
    pad(ToeBeanAsset(), ToeBeanJellyAsset(), {52.0F, 68.0F}, {180.0F, 76.0F}, {10.0F, -6.0F}, 26.0F, 35.0F, 1.07F),
    sparkle({14.0F, 22.0F}, 22.0F, 0.0F, 40.0F, 0.04),
    sparkle({198.0F, 12.0F}, 16.0F, 15.0F, -20.0F, 0.10),
    sparkle({212.0F, 168.0F}, 14.0F, -12.0F, 25.0F, 0.16),
  }.With(Frame{.width = 240.0F, .height = 230.0F});
  View card = Column {
    stage,
    Text("boops: " + std::to_string(state.boops.Get()), TextRole::Label)
        .With(Foreground(paw_ink))
        .Key("catpaw-count"),
    Text("hover to bloom · click to boop", TextRole::Label).With(Foreground(paw_muted)),
  }.With(Frame{.width = 280.0F, .height = 324.0F},
         Padding(18.0F),
         Spacing(6.0F),
         CrossAlign(CrossAxisAlignment::Center),
         Background{LinearGradient{
             .start = {0.0F, 0.0F},
             .end = {0.0F, 1.0F},
             .stops = {{0.0F, paw_card_top}, {1.0F, paw_card_bottom}},
         }},
         Border{.color = paw_border, .width = 1.0F},
         CornerRadius(24.0F),
         Shadow{.color = paw_shadow, .offset = {0.0F, 8.0F}, .blur_radius = 18.0F});
  View paw = Popped(
      Pressable(
          std::move(card),
          {.hovered_scale = 1.015F,
           .pressed_scale = 0.965F,
           .animation = SpringSpec{.stiffness = 360.0F, .damping_ratio = 0.62F}},
          [boops = state.boops, burst = state.paw_burst, pop = state.paw_pop] {
            boops += 1;
            burst = FireworksBurst{.id = burst.Get().id + 1, .local_position = {140.0F, 150.0F}};
            pop.Trigger();
          }),
      state.paw_pop);
  paw = std::move(paw)
            .On<ViewEvents::Hover>([hovered = state.paw_hovered](const HoverEvent& event) {
              hovered = event.type != HoverEventType::Leave;
            })
            .With(Fireworks{
                FireworksStyle{
                    .palette = {Color::Rgb(255, 255, 255, 0.95F), Color::Rgb(255, 178, 198, 0.95F),
                                Color::Rgb(255, 130, 160, 0.95F), Color::Rgb(255, 216, 158, 0.95F)},
                    .particle_count = 22,
                    .duration = 0.85,
                    .spread_radius = 66.0F,
                    .gravity = 30.0F,
                    .corner_radii = 24.0F,
                },
                state.paw_burst.Get(),
            })
            .Key("catpaw-demo");

  return Column {
    SectionLabel(context, "Bloom"),
    Flow {
      std::move(paw),
      Column {
        Text("Bloom cat paw", TextRole::Title),
        Text("Hover the card to spread the four toe beans and the palm pad. Click to boop the paw: it springs, "
             "and a particle burst flies out over the card.",
             TextRole::Label)
            .With(Foreground(context.colors.on_surface_variant)),
        Text("The pads, halo, and sparkles are vector assets driven by declarative animation targets.",
             TextRole::Label)
            .With(Foreground(context.colors.on_surface_variant)),
      }.With(Frame{.width = context.compact ? 240.0F : 320.0F}, Spacing(8.0F)),
    }.With(Spacing(24.0F), CrossAlign(CrossAxisAlignment::Center)),
  }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Stretch));
}

} // namespace interaction_demo
