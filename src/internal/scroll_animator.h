#pragma once

#include <huxerui/animation.h>
#include <huxerui/modifier.h>
#include <huxerui/scroll.h>

#include <algorithm>
#include <cmath>

namespace huxerui::interaction::internal {

/// Shared retained scroll animator used by ScrollToSection / ScrollToTop helpers.
/// Automatically yields to manual user scroll input if the live offset diverges from the last applied step.
class ScrollAnimator {
public:
  void Start(float from, float to, const AnimationSpec& motion) {
    if(active_ && has_applied_ && std::abs(motion_.Value() - from) <= 1.0F) {
      motion_.AnimateTo(to, motion);
    } else {
      motion_.Set(from);
      motion_.AnimateTo(to, motion);
    }
    target_ = to;
    last_applied_ = from;
    has_applied_ = true;
    active_ = true;
  }

  void Cancel() noexcept {
    active_ = false;
    has_applied_ = false;
  }

  [[nodiscard]] bool IsActive() const noexcept { return active_; }

  NodeExtension::FrameResult Advance(const ScrollController& sc, const FrameInfo& frame) {
    if(!active_ || !sc.IsConnected()) {
      Cancel();
      return {};
    }
    const ScrollMetrics metrics = sc.Metrics();
    const float live_offset = metrics.offset;
    if(has_applied_ && std::abs(live_offset - last_applied_) > 1.0F) {
      Cancel();
      motion_.Set(live_offset);
      return {};
    }
    const float max_offset = std::max(0.0F, metrics.maximum_offset);
    const float clamped_target = std::clamp(target_, 0.0F, max_offset);
    const MotionAdvanceResult result = motion_.Advance(frame);
    const float next_offset =
        !motion_.IsRunning() ? clamped_target : std::clamp(motion_.Value(), 0.0F, max_offset);
    if(result.changed || !motion_.IsRunning()) {
      (void)sc.ScrollTo(next_offset);
      last_applied_ = sc.Metrics().offset;
      has_applied_ = true;
    }
    if(!motion_.IsRunning()) {
      Cancel();
      return {};
    }
    return {result.needs_frame, result.wake_after};
  }

private:
  MotionController motion_{0.0F};
  bool active_ = false;
  bool has_applied_ = false;
  float last_applied_ = 0.0F;
  float target_ = 0.0F;
};

} // namespace huxerui::interaction::internal
