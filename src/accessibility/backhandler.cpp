#include <huxerui/interaction/accessibility/backhandler.h>

#include "../internal/validate.h"

#include <huxerui/animation.h>
#include <huxerui/layout.h>
#include <huxerui/modifier.h>
#include <huxerui/state.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {

namespace detail {
struct PredictiveBackControllerState {
  State<bool> active;
  State<float> progress;
  State<BackPhase> phase;
  bool enabled = true;
  std::function<void()> on_commit;
  std::function<void()> on_cancel;
};
} // namespace detail

namespace {

void ValidateStyle(const PredictiveBackStyle& style) {
  internal::RequireNonNegative(style.maximum_offset,
                               "HuxerUI Interaction: PredictiveBack maximum offset must be finite and non-negative");
  internal::RequirePositiveUnitInterval(
      style.minimum_scale, "HuxerUI Interaction: PredictiveBack minimum scale must be finite and in (0, 1]");
}

float ClampProgress(float progress) noexcept {
  return std::isfinite(progress) ? std::clamp(progress, 0.0F, 1.0F) : 0.0F;
}

struct BackInterceptModifier {
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  bool enabled = true;
  std::shared_ptr<const std::function<void()>> on_back;

  bool operator==(const BackInterceptModifier&) const = default;
};

class BackInterceptModifier::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const BackInterceptModifier& value) { Update(node, value); }

  void Update(ViewNode&, const BackInterceptModifier& value) {
    enabled_ = value.enabled;
    on_back_ = value.on_back;
    if(!enabled_) {
      in_transaction_ = false;
    }
  }

  [[nodiscard]] bool OnBack(ViewNode&, const BackEvent& event) override {
    if(!enabled_) {
      if(in_transaction_) {
        in_transaction_ = false;
        return true;
      }
      return false;
    }
    switch(event.phase) {
    case BackPhase::Begin:
    case BackPhase::Update:
      in_transaction_ = true;
      return true;
    case BackPhase::Cancel:
      in_transaction_ = false;
      return true;
    case BackPhase::Commit:
      in_transaction_ = false;
      if(on_back_ && *on_back_) {
        (*on_back_)();
      }
      return true;
    }
    return false;
  }

private:
  bool enabled_ = true;
  bool in_transaction_ = false;
  std::shared_ptr<const std::function<void()>> on_back_;
};

const huxerui::detail::ModifierDescriptor& BackInterceptModifier::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<BackInterceptModifier, BackInterceptModifier::Extension>();
}

struct PredictiveBackModifier {
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  std::shared_ptr<detail::PredictiveBackControllerState> state;

  bool operator==(const PredictiveBackModifier&) const = default;
};

class PredictiveBackModifier::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const PredictiveBackModifier& value) { Update(node, value); }

  void Update(ViewNode&, const PredictiveBackModifier& value) {
    if(state_ != value.state) {
      in_transaction_ = false;
    }
    state_ = value.state;
  }

  [[nodiscard]] bool OnBack(ViewNode&, const BackEvent& event) override {
    if(!state_) return false;
    if(!state_->enabled) {
      if(!in_transaction_) {
        return false;
      }
      in_transaction_ = false;
      if(state_->active.IsValid()) state_->active = false;
      if(state_->progress.IsValid()) state_->progress = 0.0F;
      if(state_->phase.IsValid()) state_->phase = BackPhase::Cancel;
      return true;
    }

    const float progress = ClampProgress(event.progress);
    switch(event.phase) {
    case BackPhase::Begin:
      in_transaction_ = true;
      if(state_->active.IsValid()) state_->active = true;
      if(state_->progress.IsValid()) state_->progress = progress;
      if(state_->phase.IsValid()) state_->phase = BackPhase::Begin;
      return true;
    case BackPhase::Update:
      in_transaction_ = true;
      if(state_->active.IsValid()) state_->active = true;
      if(state_->progress.IsValid()) state_->progress = progress;
      if(state_->phase.IsValid()) state_->phase = BackPhase::Update;
      return true;
    case BackPhase::Cancel:
      in_transaction_ = false;
      if(state_->active.IsValid()) state_->active = false;
      if(state_->progress.IsValid()) state_->progress = 0.0F;
      if(state_->phase.IsValid()) state_->phase = BackPhase::Cancel;
      if(state_->on_cancel) state_->on_cancel();
      return true;
    case BackPhase::Commit:
      in_transaction_ = false;
      if(state_->active.IsValid()) state_->active = false;
      if(state_->progress.IsValid()) state_->progress = 0.0F;
      if(state_->phase.IsValid()) state_->phase = BackPhase::Commit;
      if(state_->on_commit) state_->on_commit();
      return true;
    }
    return false;
  }

private:
  std::shared_ptr<detail::PredictiveBackControllerState> state_;
  bool in_transaction_ = false;
};

const huxerui::detail::ModifierDescriptor& PredictiveBackModifier::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<PredictiveBackModifier, PredictiveBackModifier::Extension>();
}

} // namespace

bool PredictiveBackController::IsActive() const {
  if(!state_ || !state_->active.IsValid()) return false;
  return state_->active.Get();
}

float PredictiveBackController::Progress() const {
  if(!state_ || !state_->progress.IsValid()) return 0.0F;
  return state_->progress.Get();
}

BackPhase PredictiveBackController::Phase() const {
  if(!state_ || !state_->phase.IsValid()) return BackPhase::Cancel;
  return state_->phase.Get();
}

PredictiveBackController UsePredictiveBackController() {
  auto active = UseState(false);
  auto progress = UseState(0.0F);
  auto phase = UseState(BackPhase::Cancel);
  auto holder = UseState(std::make_shared<detail::PredictiveBackControllerState>());
  auto shared = holder.Get();
  shared->active = active;
  shared->progress = progress;
  shared->phase = phase;
  return PredictiveBackController(std::move(shared));
}

View BackHandler(View content, std::function<void()> on_back, BackHandlerStyle style) {
  return std::move(content).With(BackInterceptModifier{
      .enabled = style.enabled,
      .on_back = std::make_shared<const std::function<void()>>(std::move(on_back)),
  });
}

View PredictiveBack(std::function<View(PredictiveBackSnapshot)> builder, std::function<void()> on_commit,
                    PredictiveBackStyle style, std::function<void()> on_cancel) {
  ValidateStyle(style);
  if(!builder) {
    throw std::invalid_argument("HuxerUI Interaction: PredictiveBack requires a view builder");
  }
  return Scope([builder = std::move(builder), on_commit = std::move(on_commit), style,
                on_cancel = std::move(on_cancel)]() -> View {
    auto local_active = UseState(false);
    auto local_progress = UseState(0.0F);
    auto local_phase = UseState(BackPhase::Cancel);
    auto local_holder = UseState(std::make_shared<detail::PredictiveBackControllerState>());
    std::shared_ptr<detail::PredictiveBackControllerState> state =
        style.controller.state_ ? style.controller.state_ : local_holder.Get();
    if(!style.controller.state_) {
      state->active = local_active;
      state->progress = local_progress;
      state->phase = local_phase;
    }
    state->enabled = style.enabled;
    state->on_commit = on_commit;
    state->on_cancel = on_cancel;

    const PredictiveBackSnapshot snapshot{
        .active = state->active.IsValid() ? state->active.Get() : false,
        .progress = state->progress.IsValid() ? state->progress.Get() : 0.0F,
        .phase = state->phase.IsValid() ? state->phase.Get() : BackPhase::Cancel,
    };
    return Stack{builder(snapshot)}.With(PredictiveBackModifier{.state = state});
  });
}

View PredictiveBack(View content, std::function<void()> on_commit, PredictiveBackStyle style,
                    std::function<void()> on_cancel) {
  ValidateStyle(style);
  const float max_offset = style.maximum_offset;
  const float min_scale = style.minimum_scale;
  return PredictiveBack(
      [content = std::move(content), max_offset, min_scale](PredictiveBackSnapshot snapshot) -> View {
        const float p = snapshot.active ? ClampProgress(snapshot.progress) : 0.0F;
        const float scale = 1.0F - (1.0F - min_scale) * p;
        View card = content;
        if(max_offset != 0.0F || min_scale != 1.0F) {
          card = std::move(card).With(Offset(Point{max_offset * p, 0.0F})).With(Scale(scale));
        }
        return card;
      },
      std::move(on_commit), std::move(style), std::move(on_cancel));
}

} // namespace huxerui::interaction
