#include <huxerui/interaction/fireworks.h>

#include <huxerui/animation.h>
#include <huxerui/modifier.h>
#include <huxerui/paint.h>

#include <algorithm>
#include <cmath>
#include <optional>
#include <random>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {
namespace {

// Bounds the per-frame cost of rapid retriggering.
constexpr std::size_t kMaximumBursts = 3;
constexpr float kPi = 3.14159265F;

void ValidateStyle(const FireworksStyle& style) {
  if(style.palette.empty()) {
    throw std::invalid_argument("HuxerUI Interaction: Fireworks palette must not be empty");
  }
  if(style.particle_count < 1 || style.particle_count > 64) {
    throw std::invalid_argument("HuxerUI Interaction: Fireworks particle count must be in [1, 64]");
  }
  if(!std::isfinite(style.duration) || style.duration <= 0.0) {
    throw std::invalid_argument("HuxerUI Interaction: Fireworks duration must be finite and positive");
  }
  if(!std::isfinite(style.spread_radius) || style.spread_radius <= 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: Fireworks spread radius must be finite and positive");
  }
  if(!std::isfinite(style.gravity) || style.gravity < 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: Fireworks gravity must be finite and non-negative");
  }
}

} // namespace

/// Retained paint behavior for the Fireworks modifier. The framework preserves this extension across
/// recomposition and calls Update() with the latest modifier value; a new burst id spawns one particle system.
class Fireworks::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const Fireworks& value) : rng_{std::random_device{}()} { Update(node, value); }

  void Update(ViewNode&, const Fireworks& value) {
    style_ = value.style;
    if(value.burst.id != 0 && value.burst.id != last_id_) {
      last_id_ = value.burst.id;
      Spawn(value.burst);
    }
  }

  FrameResult OnFrame(ViewNode&, const FrameInfo& frame) override {
    bool changed = false;
    bool needs_frame = false;
    std::optional<double> wake_after;
    for(auto& burst : bursts_) {
      const MotionAdvanceResult result = burst.controller.Advance(frame);
      changed |= result.changed;
      needs_frame |= result.needs_frame;
      if(result.wake_after.has_value()) {
        wake_after = wake_after.has_value() ? std::min(*wake_after, *result.wake_after) : *result.wake_after;
      }
    }
    const auto settled = std::remove_if(bursts_.begin(), bursts_.end(),
                                        [](const Burst& burst) {
                                          return !burst.controller.IsRunning()
                                              && burst.controller.Value() >= 1.0F;
                                        });
    if(settled != bursts_.end()) {
      bursts_.erase(settled, bursts_.end());
      changed = true;
    }
    if(changed) InvalidatePaint(PaintInvalidation::Foreground);
    return {needs_frame, wake_after};
  }

  void PaintAboveContent(const ViewNode& node, PaintContext& context) const override {
    if(bursts_.empty()) return;
    const Rect bounds = node.Bounds();
    if(bounds.IsEmpty()) return;
    context.PushClip(bounds, style_.corner_radii);
    for(const auto& burst : bursts_) {
      const float progress = burst.controller.Value();
      if(progress <= 0.0F || progress >= 1.0F) continue;
      for(const auto& particle : burst.particles) {
        const float local = std::clamp((progress - particle.delay) / (1.0F - particle.delay), 0.0F, 1.0F);
        if(local <= 0.0F) continue;
        const float eased = 1.0F - (1.0F - local) * (1.0F - local);
        const Point position{
            burst.origin.x + particle.direction.x * particle.distance * eased,
            burst.origin.y + particle.direction.y * particle.distance * eased
                + style_.gravity * local * local,
        };
        Color color = particle.color;
        color.alpha *= 1.0F - local;
        if(color.alpha <= 0.0F) continue;
        context.DrawCircle(position, particle.radius * (1.0F - 0.5F * local), color);
      }
    }
    context.PopClip();
  }

private:
  struct Particle {
    Point direction;
    float distance = 0.0F;
    float radius = 0.0F;
    float delay = 0.0F;
    Color color;
  };

  struct Burst {
    Point origin;
    MotionController controller{0.0F};
    std::vector<Particle> particles;
  };

  void Spawn(const FireworksBurst& request) {
    if(bursts_.size() >= kMaximumBursts) bursts_.erase(bursts_.begin());
    Burst burst;
    burst.origin = request.local_position;
    burst.controller.AnimateTo(1.0F, TweenSpec{.duration = style_.duration, .easing = Easing::Linear});
    std::uniform_real_distribution<float> angle(0.0F, 2.0F * kPi);
    std::uniform_real_distribution<float> distance(0.35F, 1.0F);
    std::uniform_real_distribution<float> radius(1.5F, 3.0F);
    std::uniform_real_distribution<float> delay(0.0F, 0.2F);
    std::uniform_int_distribution<std::size_t> palette(0, style_.palette.size() - 1);
    burst.particles.reserve(static_cast<std::size_t>(style_.particle_count));
    for(int index = 0; index < style_.particle_count; ++index) {
      const float radians = angle(rng_);
      burst.particles.push_back(Particle{
          .direction = {std::cos(radians), std::sin(radians)},
          .distance = distance(rng_) * style_.spread_radius,
          .radius = radius(rng_),
          .delay = delay(rng_),
          .color = style_.palette[palette(rng_)],
      });
    }
    bursts_.push_back(std::move(burst));
    InvalidatePaint(PaintInvalidation::Foreground);
  }

  FireworksStyle style_;
  std::uint64_t last_id_ = 0;
  std::mt19937 rng_;
  std::vector<Burst> bursts_;
};

const detail::ModifierDescriptor& Fireworks::Descriptor() {
  return detail::ModifierDescriptorFor<Fireworks, Fireworks::Extension>();
}

Fireworks::Fireworks(FireworksStyle style, FireworksBurst burst) {
  ValidateStyle(style);
  this->style = std::move(style);
  this->burst = burst;
}

} // namespace huxerui::interaction
