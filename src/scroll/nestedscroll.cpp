#include <huxerui/interaction/scroll/nestedscroll.h>

#include "../internal/validate.h"

#include <huxerui/state.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

namespace huxerui::interaction {

namespace detail {
struct ScrollHideControllerState {
  State<float> hidden_offset;
  ScrollHideStyle style;
  NestedScrollConnection connection;
};
} // namespace detail

namespace {

void ValidateStyle(const ScrollHideStyle& style) {
  internal::RequirePositive(style.extent,
                            "HuxerUI Interaction: ScrollHideStyle extent must be finite and positive");
}

float ClampConsumption(float available, float requested) noexcept {
  if(!std::isfinite(available) || !std::isfinite(requested) || available == 0.0F || requested == 0.0F) {
    return 0.0F;
  }
  const float clamped = available > 0.0F ? std::clamp(requested, 0.0F, available)
                                         : std::clamp(requested, available, 0.0F);
  return clamped == 0.0F ? 0.0F : clamped;
}

} // namespace

class NestedScrollConnection::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const NestedScrollConnection& value) { Update(node, value); }

  void Update(ViewNode&, const NestedScrollConnection& value) { callbacks_ = value.callbacks; }

  float OnPreScroll(ViewNode&, Axis axis, float available, ScrollSource source) override {
    if(!callbacks_ || !callbacks_->on_pre_scroll) return 0.0F;
    return ClampConsumption(available, callbacks_->on_pre_scroll(axis, available, source));
  }

  float OnPostScroll(ViewNode&, Axis axis, float consumed, float available, ScrollSource source) override {
    if(!callbacks_ || !callbacks_->on_post_scroll) return 0.0F;
    return ClampConsumption(available, callbacks_->on_post_scroll(axis, consumed, available, source));
  }

  float OnPreFling(ViewNode&, Axis axis, float available_velocity) override {
    if(!callbacks_ || !callbacks_->on_pre_fling) return 0.0F;
    return ClampConsumption(available_velocity, callbacks_->on_pre_fling(axis, available_velocity));
  }

  float OnPostFling(ViewNode&, Axis axis, float consumed_velocity, float available_velocity) override {
    if(!callbacks_ || !callbacks_->on_post_fling) return 0.0F;
    return ClampConsumption(available_velocity,
                            callbacks_->on_post_fling(axis, consumed_velocity, available_velocity));
  }

private:
  std::shared_ptr<const NestedScrollCallbacks> callbacks_;
};

const huxerui::detail::ModifierDescriptor& NestedScrollConnection::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<NestedScrollConnection, NestedScrollConnection::Extension>();
}

NestedScrollConnection ScrollHideController::Connection() const {
  return state_ ? state_->connection : NestedScrollConnection{};
}

float ScrollHideController::HiddenOffset() const {
  if(!state_ || !state_->hidden_offset.IsValid()) return 0.0F;
  return std::clamp(state_->hidden_offset.Get(), 0.0F, state_->style.extent);
}

float ScrollHideController::Fraction() const {
  if(!state_ || !state_->hidden_offset.IsValid() || state_->style.extent <= 0.0F) return 0.0F;
  return std::clamp(HiddenOffset() / state_->style.extent, 0.0F, 1.0F);
}

bool ScrollHideController::IsHidden() const {
  if(!state_ || !state_->hidden_offset.IsValid() || state_->style.extent <= 0.0F) return false;
  return Fraction() >= 1.0F - 1e-4F;
}

void ScrollHideController::Show() const {
  if(state_ && state_->hidden_offset.IsValid()) {
    state_->hidden_offset = 0.0F;
  }
}

void ScrollHideController::Hide() const {
  if(state_ && state_->hidden_offset.IsValid()) {
    state_->hidden_offset = state_->style.extent;
  }
}

ScrollHideController UseScrollHideController(ScrollHideStyle style) {
  ValidateStyle(style);
  auto hidden_offset = UseState(0.0F);
  auto holder = UseState(std::make_shared<detail::ScrollHideControllerState>());
  auto shared = holder.Get();
  shared->hidden_offset = hidden_offset;
  shared->style = style;
  if(!shared->connection.callbacks) {
    std::weak_ptr<detail::ScrollHideControllerState> weak_state = shared;
    shared->connection = NestedScrollConnection(NestedScrollCallbacks{
        .on_pre_scroll =
            [weak_state](Axis axis, float available, ScrollSource) -> float {
          auto state = weak_state.lock();
          if(!state || axis != state->style.axis || !state->hidden_offset.IsValid()) return 0.0F;
          if(!std::isfinite(available) || available == 0.0F) return 0.0F;
          const float extent = state->style.extent;
          const float current = std::clamp(state->hidden_offset.Get(), 0.0F, extent);
          const float next = std::clamp(current + available, 0.0F, extent);
          const float delta = next - current;
          if(delta != 0.0F) {
            state->hidden_offset = next;
          }
          return state->style.consume_scroll ? delta : 0.0F;
        },
        .on_post_scroll = {},
        .on_pre_fling = {},
        .on_post_fling = {},
    });
  }
  return ScrollHideController(std::move(shared));
}

} // namespace huxerui::interaction
