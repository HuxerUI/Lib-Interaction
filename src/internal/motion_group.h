#pragma once

#include <huxerui/animation.h>
#include <huxerui/modifier.h>

#include <algorithm>
#include <optional>

namespace huxerui::interaction::internal {

/// Aggregates Advance() results across multiple retained MotionControllers in one OnFrame() pass.
struct FrameAccumulator {
  bool changed = false;
  bool needs_frame = false;
  std::optional<double> wake_after;

  void Add(const MotionAdvanceResult& result) noexcept {
    changed |= result.changed;
    needs_frame |= result.needs_frame;
    if(result.wake_after.has_value()) {
      wake_after = wake_after.has_value() ? std::min(*wake_after, *result.wake_after) : *result.wake_after;
    }
  }

  [[nodiscard]] NodeExtension::FrameResult Result(bool keep_awake = false) const noexcept {
    return {needs_frame || keep_awake, wake_after};
  }
};

} // namespace huxerui::interaction::internal
