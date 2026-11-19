#pragma once

#include <huxerui/event.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <functional>
#include <memory>
#include <utility>

namespace huxerui::interaction {
namespace detail {
struct PredictiveBackControllerState;
} // namespace detail

/// Configuration for BackHandler.
struct BackHandlerStyle {
  /// When false, platform Back events pass through to enclosing BackHandler/PredictiveBack scopes or platform
  /// defaults.
  bool enabled = true;

  bool operator==(const BackHandlerStyle&) const = default;
};

/// Intercepts committed platform Back requests (including Escape key when unfocused/unhandled by editors) via
/// `NodeExtension::OnBack`. Innermost enabled handlers win; disabled handlers pass Back through outward.
View BackHandler(View content, std::function<void()> on_back, BackHandlerStyle style = {});

/// Pipeable adaptor for BackHandler.
[[nodiscard]] inline ViewAdaptor BackHandler(std::function<void()> on_back, BackHandlerStyle style = {}) {
  return ViewAdaptor(
      [on_back = std::move(on_back), style](View content) { return BackHandler(std::move(content), on_back, style); });
}

/// Live predictive Back gesture state supplied to the headless PredictiveBack builder.
struct PredictiveBackSnapshot {
  /// True while a Begin/Update predictive transaction is active.
  bool active = false;
  /// Normalized gesture progress in `[0, 1]`.
  float progress = 0.0F;
  /// Most recent BackPhase received by this scope.
  BackPhase phase = BackPhase::Cancel;

  bool operator==(const PredictiveBackSnapshot&) const = default;
};

struct PredictiveBackStyle;

/// Copyable handle exposing live predictive Back gesture state. Default-constructed handles are unbound and safe
/// outside an Application runtime; bind one with UsePredictiveBackController().
class PredictiveBackController {
public:
  PredictiveBackController() noexcept = default;

  [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(state_); }

  /// True while a predictive Back transaction is in progress (subscribes active composition).
  [[nodiscard]] bool IsActive() const;

  /// Current normalized predictive Back progress in `[0, 1]` (subscribes active composition).
  [[nodiscard]] float Progress() const;

  /// Most recent BackPhase observed by this controller (subscribes active composition).
  [[nodiscard]] BackPhase Phase() const;

  bool operator==(const PredictiveBackController&) const = default;

private:
  explicit PredictiveBackController(std::shared_ptr<detail::PredictiveBackControllerState> state)
      : state_(std::move(state)) {}

  std::shared_ptr<detail::PredictiveBackControllerState> state_;

  friend PredictiveBackController UsePredictiveBackController();
  friend View PredictiveBack(std::function<View(PredictiveBackSnapshot)> builder, std::function<void()> on_commit,
                             PredictiveBackStyle style, std::function<void()> on_cancel);
};

/// Remembers a PredictiveBackController in the current composition scope.
PredictiveBackController UsePredictiveBackController();

/// Configuration and default preview motion for PredictiveBack.
struct PredictiveBackStyle {
  /// When false, Back requests bypass this scope unless a gesture transaction is already captured.
  bool enabled = true;
  /// Horizontal slide offset at `progress == 1` in logical pixels (wrapped overload). Must be finite and non-negative.
  float maximum_offset = 24.0F;
  /// Minimum uniform scale factor at `progress == 1` (wrapped overload). Must be finite and in `(0, 1]`.
  float minimum_scale = 0.94F;
  /// Optional external controller updated across Begin/Update/Cancel/Commit phases.
  PredictiveBackController controller{};

  bool operator==(const PredictiveBackStyle&) const = default;
};

/// Headless predictive Back builder delegating gesture presentation to `builder(PredictiveBackSnapshot)`.
/// @throws std::invalid_argument If `builder` is empty or `style` bounds are invalid.
View PredictiveBack(std::function<View(PredictiveBackSnapshot)> builder, std::function<void()> on_commit,
                    PredictiveBackStyle style = {}, std::function<void()> on_cancel = {});

/// Wraps `content` with predictive Back handling and subtle progress-driven offset/scale preview motion.
/// @throws std::invalid_argument If `style` bounds are invalid.
View PredictiveBack(View content, std::function<void()> on_commit, PredictiveBackStyle style = {},
                    std::function<void()> on_cancel = {});

/// Pipeable adaptor for PredictiveBack.
[[nodiscard]] inline ViewAdaptor PredictiveBack(std::function<void()> on_commit, PredictiveBackStyle style = {},
                                                std::function<void()> on_cancel = {}) {
  return ViewAdaptor(
      [on_commit = std::move(on_commit), style, on_cancel = std::move(on_cancel)](View content) {
        return PredictiveBack(std::move(content), on_commit, style, on_cancel);
      });
}

} // namespace huxerui::interaction
