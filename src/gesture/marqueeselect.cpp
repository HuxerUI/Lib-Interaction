#include <huxerui/interaction/gesture/marqueeselect.h>

#include "../internal/validate.h"

#include <huxerui/environment.h>
#include <huxerui/gesture.h>
#include <huxerui/layout.h>
#include <huxerui/lifecycle.h>
#include <huxerui/modifier.h>
#include <huxerui/paint.h>
#include <huxerui/state.h>
#include <huxerui/vector.h>

#include <algorithm>
#include <cstdint>
#include <map>
#include <utility>

namespace huxerui::interaction {

namespace detail {

struct MarqueeItemEntry {
  Rect window_bounds{};
  std::uint64_t token = 0;
  bool has_bounds = false;
};

struct MarqueeSelectControllerState {
  State<bool> dragging;
  State<std::vector<std::size_t>> selection;
  MarqueeSelectStyle style;
  std::function<void(const std::vector<std::size_t>&)> on_selection_changed;
  std::function<void(const std::vector<std::size_t>&)> on_selection_committed;
  std::map<std::size_t, MarqueeItemEntry> items;
  std::uint64_t next_token = 1;
  Rect container_bounds{};
  std::optional<Rect> active_rect;
  std::vector<std::size_t> pre_drag_selection;
  std::function<void()> invalidate_overlay;
};

} // namespace detail

namespace {

void ValidateStyle(const MarqueeSelectStyle& style) {
  internal::RequireNonNegative(style.border_width,
                               "HuxerUI Interaction: MarqueeSelect border width must be finite and non-negative");
  internal::RequireOptionalPositive(
      style.minimum_distance, "HuxerUI Interaction: MarqueeSelect minimum distance must be finite and positive");
  internal::RequireNonNegative(style.corner_radii.top_left,
                               "HuxerUI Interaction: MarqueeSelect corner radii must be finite and non-negative");
  internal::RequireNonNegative(style.corner_radii.top_right,
                               "HuxerUI Interaction: MarqueeSelect corner radii must be finite and non-negative");
  internal::RequireNonNegative(style.corner_radii.bottom_right,
                               "HuxerUI Interaction: MarqueeSelect corner radii must be finite and non-negative");
  internal::RequireNonNegative(style.corner_radii.bottom_left,
                               "HuxerUI Interaction: MarqueeSelect corner radii must be finite and non-negative");
}

std::vector<std::size_t> NormalizeSelection(std::vector<std::size_t> indices) {
  std::sort(indices.begin(), indices.end());
  indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
  return indices;
}

void ApplySelection(detail::MarqueeSelectControllerState& state, std::vector<std::size_t> normalized) {
  if(!state.selection.IsValid()) return;
  if(state.selection.Get() == normalized) return;
  state.selection = normalized;
  if(state.on_selection_changed) {
    state.on_selection_changed(normalized);
  }
}

bool RectsIntersect(const Rect& a, const Rect& b) noexcept {
  return a.x < b.x + b.width && a.x + a.width > b.x && a.y < b.y + b.height && a.y + a.height > b.y;
}

struct MarqueeSelectEnvironment {
  std::shared_ptr<detail::MarqueeSelectControllerState> state;

  static MarqueeSelectEnvironment Default() { return {}; }

  bool operator==(const MarqueeSelectEnvironment&) const = default;
};

struct MarqueeTokenHolder {
  std::uint64_t token = 0;
};

struct MarqueeOverlayLayer {
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  std::shared_ptr<detail::MarqueeSelectControllerState> state;
  Color fill_color{};
  Color border_color{};
  float border_width = 1.5F;
  CornerRadii corner_radii{4.0F};

  bool operator==(const MarqueeOverlayLayer&) const = default;
};

class MarqueeOverlayLayer::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const MarqueeOverlayLayer& value) { Update(node, value); }

  ~Extension() override {
    if(state_) {
      state_->invalidate_overlay = {};
    }
  }

  void Update(ViewNode&, const MarqueeOverlayLayer& value) {
    if(state_ && state_ != value.state) {
      state_->invalidate_overlay = {};
    }
    state_ = value.state;
    fill_color_ = value.fill_color;
    border_color_ = value.border_color;
    border_width_ = value.border_width;
    corner_radii_ = value.corner_radii;
    if(state_) {
      state_->invalidate_overlay = [this] { InvalidatePaint(PaintInvalidation::Foreground); };
    }
    InvalidatePaint(PaintInvalidation::Foreground);
  }

  PaintInvalidation PrepareGeometry(ViewNode& node, TextMeasurer&) override {
    if(state_) {
      state_->container_bounds = node.Bounds();
    }
    return PaintInvalidation::None;
  }

  void PaintAboveContent(const ViewNode&, PaintContext& context) const override {
    if(!state_ || !state_->active_rect.has_value()) return;
    const Rect rect = *state_->active_rect;
    if(rect.width <= 0.0F || rect.height <= 0.0F) return;
    if(fill_color_.alpha > 0.0F) {
      context.DrawRect(rect, Brush(fill_color_), corner_radii_);
    }
    if(border_width_ > 0.0F && border_color_.alpha > 0.0F) {
      context.DrawBorder(rect, border_color_, StrokeStyle{.width = border_width_}, corner_radii_);
    }
  }

private:
  std::shared_ptr<detail::MarqueeSelectControllerState> state_;
  Color fill_color_{};
  Color border_color_{};
  float border_width_ = 1.5F;
  CornerRadii corner_radii_{4.0F};
};

const huxerui::detail::ModifierDescriptor& MarqueeOverlayLayer::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<MarqueeOverlayLayer, MarqueeOverlayLayer::Extension>();
}

struct MarqueeItemProbe {
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  std::shared_ptr<detail::MarqueeSelectControllerState> state;
  std::size_t index = 0;
  std::uint64_t token = 0;

  bool operator==(const MarqueeItemProbe&) const = default;
};

class MarqueeItemProbe::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const MarqueeItemProbe& value) { Update(node, value); }

  void Update(ViewNode&, const MarqueeItemProbe& value) {
    state_ = value.state;
    index_ = value.index;
    token_ = value.token;
  }

  PaintInvalidation PrepareGeometry(ViewNode& node, TextMeasurer&) override {
    if(!state_) return PaintInvalidation::None;
    const auto it = state_->items.find(index_);
    if(it != state_->items.end() && it->second.token == token_) {
      const Rect bounds = node.PresentationBounds();
      it->second.window_bounds = bounds;
      it->second.has_bounds = !bounds.IsEmpty();
    }
    return PaintInvalidation::None;
  }

private:
  std::shared_ptr<detail::MarqueeSelectControllerState> state_;
  std::size_t index_ = 0;
  std::uint64_t token_ = 0;
};

const huxerui::detail::ModifierDescriptor& MarqueeItemProbe::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<MarqueeItemProbe, MarqueeItemProbe::Extension>();
}

Rect BuildLocalMarqueeRect(const detail::MarqueeSelectControllerState& state, const DragEvent& event) noexcept {
  auto clamp_point = [&](Point p) -> Point {
    if(!state.style.clamp_to_bounds || state.container_bounds.IsEmpty()) return p;
    return {
        std::clamp(p.x, state.container_bounds.x, state.container_bounds.x + state.container_bounds.width),
        std::clamp(p.y, state.container_bounds.y, state.container_bounds.y + state.container_bounds.height),
    };
  };
  const Point p0 = clamp_point(event.origin);
  const Point p1 = clamp_point(event.position);
  const float min_x = std::min(p0.x, p1.x);
  const float min_y = std::min(p0.y, p1.y);
  return {
      min_x,
      min_y,
      std::max(0.0F, std::max(p0.x, p1.x) - min_x),
      std::max(0.0F, std::max(p0.y, p1.y) - min_y),
  };
}

std::vector<std::size_t> ComputeDragSelection(const detail::MarqueeSelectControllerState& state, const Rect& local_rect,
                                              const DragEvent& event) {
  const Point to_window = event.window_position - event.position;
  const Rect window_rect{local_rect.x + to_window.x, local_rect.y + to_window.y, local_rect.width, local_rect.height};
  std::vector<std::size_t> result;
  if(state.style.additive) {
    result = state.pre_drag_selection;
  }
  for(const auto& [index, entry] : state.items) {
    if(entry.has_bounds && RectsIntersect(window_rect, entry.window_bounds)) {
      result.push_back(index);
    }
  }
  return NormalizeSelection(std::move(result));
}

} // namespace

bool MarqueeSelectController::IsDragging() const {
  if(!state_ || !state_->dragging.IsValid()) return false;
  return state_->dragging.Get();
}

std::vector<std::size_t> MarqueeSelectController::Selection() const {
  if(!state_ || !state_->selection.IsValid()) return {};
  return state_->selection.Get();
}

bool MarqueeSelectController::IsSelected(std::size_t index) const {
  const std::vector<std::size_t> current = Selection();
  return std::binary_search(current.begin(), current.end(), index);
}

void MarqueeSelectController::SetSelection(std::vector<std::size_t> indices) const {
  if(!state_) return;
  ApplySelection(*state_, NormalizeSelection(std::move(indices)));
}

void MarqueeSelectController::Toggle(std::size_t index) const {
  if(!state_ || !state_->selection.IsValid()) return;
  std::vector<std::size_t> next = state_->selection.Get();
  const auto it = std::lower_bound(next.begin(), next.end(), index);
  if(it != next.end() && *it == index) {
    next.erase(it);
  } else {
    next.insert(it, index);
  }
  ApplySelection(*state_, std::move(next));
}

void MarqueeSelectController::Clear() const {
  if(!state_) return;
  ApplySelection(*state_, {});
}

MarqueeSelectController UseMarqueeSelectController(std::vector<std::size_t> initial_selection) {
  auto dragging = UseState(false);
  auto selection = UseState(NormalizeSelection(std::move(initial_selection)));
  auto holder = UseState(std::make_shared<detail::MarqueeSelectControllerState>());
  auto shared = holder.Get();
  shared->dragging = dragging;
  shared->selection = selection;
  return MarqueeSelectController(std::move(shared));
}

View MarqueeSelectArea(View content, MarqueeSelectStyle style,
                       std::function<void(const std::vector<std::size_t>&)> on_selection_changed,
                       std::function<void(const std::vector<std::size_t>&)> on_selection_committed) {
  ValidateStyle(style);
  return Scope([content = std::move(content), style, on_selection_changed = std::move(on_selection_changed),
                on_selection_committed = std::move(on_selection_committed)]() -> View {
    auto local_dragging = UseState(false);
    auto local_selection = UseState(std::vector<std::size_t>{});
    auto local_holder = UseState(std::make_shared<detail::MarqueeSelectControllerState>());
    std::shared_ptr<detail::MarqueeSelectControllerState> state =
        style.controller.state_ ? style.controller.state_ : local_holder.Get();
    if(!style.controller.state_) {
      state->dragging = local_dragging;
      state->selection = local_selection;
    }
    state->style = style;
    state->style.controller = {};
    state->on_selection_changed = on_selection_changed;
    state->on_selection_committed = on_selection_committed;

    auto update_from_event = [state](const DragEvent& event) {
      const Rect local_rect = BuildLocalMarqueeRect(*state, event);
      state->active_rect = local_rect;
      if(state->invalidate_overlay) state->invalidate_overlay();
      ApplySelection(*state, ComputeDragSelection(*state, local_rect, event));
    };

    View area = ProvideEnvironment(MarqueeSelectEnvironment{state}, content);
    return Stack{std::move(area)}
        .With(MarqueeOverlayLayer{
            .state = state,
            .fill_color = style.fill_color,
            .border_color = style.border_color,
            .border_width = style.border_width,
            .corner_radii = style.corner_radii,
        })
        .With(DragGesture{
            .axis = std::nullopt,
            .minimum_distance = style.minimum_distance,
            .minimum_press_duration = std::nullopt,
        })
        .On<DragEvents::Started>([state, update_from_event](const DragEvent& event) {
          state->pre_drag_selection = state->selection.IsValid() ? state->selection.Get() : std::vector<std::size_t>{};
          if(state->dragging.IsValid()) state->dragging = true;
          update_from_event(event);
        })
        .On<DragEvents::Changed>([update_from_event](const DragEvent& event) { update_from_event(event); })
        .On<DragEvents::Ended>([state](const DragEvent& event) {
          const Rect local_rect = BuildLocalMarqueeRect(*state, event);
          const std::vector<std::size_t> final_selection = ComputeDragSelection(*state, local_rect, event);
          state->active_rect.reset();
          if(state->invalidate_overlay) state->invalidate_overlay();
          if(state->dragging.IsValid()) state->dragging = false;
          ApplySelection(*state, final_selection);
          if(state->on_selection_committed) {
            state->on_selection_committed(final_selection);
          }
        })
        .On<DragEvents::Canceled>([state](const DragEvent&) {
          state->active_rect.reset();
          if(state->invalidate_overlay) state->invalidate_overlay();
          if(state->dragging.IsValid()) state->dragging = false;
          ApplySelection(*state, state->pre_drag_selection);
        });
  });
}

View MarqueeSelectItem(View content, std::size_t index) {
  return Scope([content = std::move(content), index]() -> View {
    const auto& env = UseEnvironment<MarqueeSelectEnvironment>();
    auto token_holder = UseState(std::make_shared<MarqueeTokenHolder>()).Get();
    View item = content;
    if(env.state) {
      if(token_holder->token == 0) {
        token_holder->token = env.state->next_token++;
      }
      const std::uint64_t token = token_holder->token;
      const auto existing = env.state->items.find(index);
      if(existing == env.state->items.end() || existing->second.token != token) {
        env.state->items.insert_or_assign(index, detail::MarqueeItemEntry{.token = token});
      }
      std::weak_ptr<detail::MarqueeSelectControllerState> state_weak = env.state;
      Lifecycle(
          [state_weak, index, token]() {
            return [state_weak, index, token]() {
              if(auto state = state_weak.lock()) {
                const auto it = state->items.find(index);
                if(it != state->items.end() && it->second.token == token) {
                  state->items.erase(it);
                }
              }
            };
          },
          index, token);
      item = std::move(item).With(MarqueeItemProbe{.state = env.state, .index = index, .token = token});
    }
    return item;
  });
}

} // namespace huxerui::interaction
