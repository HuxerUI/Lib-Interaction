#include <huxerui/interaction/dismissible.h>

#include <huxerui/modifier.h>
#include <huxerui/state.h>
#include <huxerui/task.h>

#include <cmath>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const DismissibleStyle& style) {
  if(!std::isfinite(style.dismiss_threshold) || style.dismiss_threshold <= 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: Dismissible threshold must be finite and positive");
  }
  if(!std::isfinite(style.minimum_velocity) || style.minimum_velocity < 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: Dismissible velocity must be finite and non-negative");
  }
  if(!std::isfinite(style.exit_distance) || style.exit_distance <= 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: Dismissible exit distance must be finite and positive");
  }
  if(!std::isfinite(style.exit_duration) || style.exit_duration < 0.0) {
    throw std::invalid_argument("HuxerUI Interaction: Dismissible exit duration must be finite and non-negative");
  }
}

float AxisComponent(Point point, Axis axis) { return axis == Axis::Horizontal ? point.x : point.y; }

Point AxisPoint(float value, Axis axis) {
  return axis == Axis::Horizontal ? Point{value, 0.0F} : Point{0.0F, value};
}

} // namespace

View Dismissible(View content, std::function<void()> on_dismissed, DismissibleStyle style) {
  ValidateStyle(style);
  return Scope([content = std::move(content), on_dismissed = std::move(on_dismissed), style]() -> View {
    TaskScope tasks = UseTaskScope();
    auto offset = UseState(Point{});
    auto dragging = UseState(false);
    auto dismissing = UseState(false);
    auto generation = UseState<std::uint64_t>(0);
    const AnimationSpec motion = dismissing.Get()
                                      ? style.exit_motion
                                      : (dragging.Get() ? style.follow_motion : style.settle_motion);
    View moving = content;
    moving = std::move(moving).With(Offset(AnimateTo(offset.Get(), motion)));
    View surface = Stack{std::move(moving)}.With(Background{style.background}, ClipChildren(),
                                                  DragGesture{.axis = style.axis});
    return std::move(surface)
        .On<DragEvents::Started>([dragging, generation](const DragEvent&) {
          dragging = true;
          generation = generation.Get() + 1U;
        })
        .On<DragEvents::Changed>([dragging, dismissing, offset, style](const DragEvent& event) {
          if(!dragging.Get() || dismissing.Get()) return;
          offset = AxisPoint(AxisComponent(event.translation, style.axis), style.axis);
        })
        .On<DragEvents::Ended>(
            [dragging, dismissing, offset, generation, tasks, style,
             on_dismissed](const DragEvent& event) {
              if(!dragging.Get() || dismissing.Get()) return;
              dragging = false;
              const float travel = AxisComponent(event.translation, style.axis);
              const float velocity = AxisComponent(event.velocity, style.axis);
              const bool crosses_distance = std::abs(travel) >= style.dismiss_threshold;
              const bool crosses_velocity =
                  std::abs(velocity) >= style.minimum_velocity && (travel == 0.0F || (travel > 0.0F) == (velocity > 0.0F));
              if(!crosses_distance && !crosses_velocity) {
                offset = Point{};
                return;
              }
              const float direction = travel != 0.0F ? (travel > 0.0F ? 1.0F : -1.0F)
                                                     : (velocity >= 0.0F ? 1.0F : -1.0F);
              offset = AxisPoint(direction * style.exit_distance, style.axis);
              dismissing = true;
              const std::uint64_t token = generation.Get() + 1U;
              generation = token;
              tasks.Launch([offset, dismissing, generation, token, style, on_dismissed]() -> Task<void> {
                co_await Delay(std::chrono::duration<double>(style.exit_duration));
                if(generation.Get() != token) co_return;
                dismissing = false;
                if(on_dismissed) on_dismissed();
              });
            })
        .On<DragEvents::Canceled>([dragging, dismissing, offset, generation](const DragEvent&) {
          dragging = false;
          dismissing = false;
          offset = Point{};
          generation = generation.Get() + 1U;
        });
  });
}

} // namespace huxerui::interaction
