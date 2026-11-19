#include <huxerui/interaction/scroll/scrollspy.h>

#include "../internal/validate.h"

#include <huxerui/environment.h>
#include <huxerui/layout.h>
#include <huxerui/lifecycle.h>
#include <huxerui/modifier.h>
#include <huxerui/state.h>
#include <huxerui/task.h>

#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <utility>
#include <vector>

namespace huxerui::interaction {

namespace detail {

struct ScrollSpySectionEntry {
  Rect window_bounds{};
  std::uint64_t token = 0;
  bool has_bounds = false;
};

struct ScrollSpyControllerState {
  State<std::size_t> active_section;
  State<std::size_t> section_count;
  ScrollSpyStyle style;
  std::function<void(std::size_t)> on_active_changed;
  std::map<std::size_t, ScrollSpySectionEntry> sections;
  std::uint64_t next_token = 1;
  Rect viewport_bounds{};
  bool has_viewport = false;
  bool needs_sync = false;
  bool sync_scheduled = false;
  std::shared_ptr<bool> alive;
  std::function<void()> post_sync;
};

} // namespace detail

namespace {

void ValidateStyle(const ScrollSpyStyle& style) {
  internal::RequireNonNegative(style.activation_offset,
                               "HuxerUI Interaction: ScrollSpy activation offset must be finite and non-negative");
}

struct ScrollSpyEnvironment {
  std::shared_ptr<detail::ScrollSpyControllerState> state;

  static ScrollSpyEnvironment Default() { return {}; }

  bool operator==(const ScrollSpyEnvironment&) const = default;
};

struct ScrollSpyTokenHolder {
  std::uint64_t token = 0;
};

void ApplyActiveSection(detail::ScrollSpyControllerState& state, std::size_t next) {
  if(!state.active_section.IsValid()) return;
  if(state.active_section.Get() == next) return;
  state.active_section = next;
  if(state.on_active_changed) {
    state.on_active_changed(next);
  }
}

std::optional<std::size_t> ComputeActiveSection(const detail::ScrollSpyControllerState& state) {
  if(!state.has_viewport) return std::nullopt;
  const bool vertical = state.style.axis == Axis::Vertical;
  struct MeasuredSection {
    std::size_t index = 0;
    float start = 0.0F;
  };
  std::vector<MeasuredSection> entries;
  entries.reserve(state.sections.size());
  for(const auto& [index, entry] : state.sections) {
    if(!entry.has_bounds) continue;
    entries.push_back({index, vertical ? entry.window_bounds.y : entry.window_bounds.x});
  }
  if(entries.empty()) return std::nullopt;
  std::sort(entries.begin(), entries.end(), [](const MeasuredSection& a, const MeasuredSection& b) {
    return a.start < b.start || (a.start == b.start && a.index < b.index);
  });

  const float vp_start = vertical ? state.viewport_bounds.y : state.viewport_bounds.x;
  const float vp_end = vp_start + (vertical ? state.viewport_bounds.height : state.viewport_bounds.width);

  if(state.style.scroll_controller.has_value() && state.style.scroll_controller->IsConnected()) {
    const ScrollMetrics metrics = state.style.scroll_controller->Metrics();
    if(metrics.axis == state.style.axis && metrics.maximum_offset > 0.0F
       && metrics.offset >= metrics.maximum_offset - 1.0F && entries.back().start < vp_end) {
      return entries.back().index;
    }
  }

  const float threshold = vp_start + state.style.activation_offset + 0.5F;
  std::optional<std::size_t> candidate;
  for(const MeasuredSection& entry : entries) {
    if(entry.start <= threshold) {
      candidate = entry.index;
    } else {
      break;
    }
  }
  return candidate.value_or(entries.front().index);
}

void ScheduleSync(detail::ScrollSpyControllerState& state) {
  if(state.sync_scheduled || !state.alive || !*state.alive || !state.post_sync) return;
  state.sync_scheduled = true;
  state.post_sync();
}

struct ScrollSpyViewportProbe {
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  std::shared_ptr<detail::ScrollSpyControllerState> state;
  Axis axis = Axis::Vertical;
  float activation_offset = 16.0F;

  bool operator==(const ScrollSpyViewportProbe&) const = default;
};

class ScrollSpyViewportProbe::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const ScrollSpyViewportProbe& value) { Update(node, value); }

  void Update(ViewNode&, const ScrollSpyViewportProbe& value) {
    const bool config_changed =
        state_ != value.state || axis_ != value.axis || activation_offset_ != value.activation_offset;
    state_ = value.state;
    axis_ = value.axis;
    activation_offset_ = value.activation_offset;
    if(config_changed && state_) {
      state_->needs_sync = true;
    }
  }

  PaintInvalidation PrepareGeometry(ViewNode& node, TextMeasurer&) override {
    if(!state_) return PaintInvalidation::None;
    const Rect bounds = node.PresentationBounds();
    const bool valid = !bounds.IsEmpty();
    const bool bounds_changed = state_->has_viewport != valid || state_->viewport_bounds != bounds;
    if(bounds_changed) {
      state_->has_viewport = valid;
      state_->viewport_bounds = bounds;
    }
    if(bounds_changed || state_->needs_sync) {
      state_->needs_sync = false;
      ScheduleSync(*state_);
    }
    return PaintInvalidation::None;
  }

private:
  std::shared_ptr<detail::ScrollSpyControllerState> state_;
  Axis axis_ = Axis::Vertical;
  float activation_offset_ = 16.0F;
};

const huxerui::detail::ModifierDescriptor& ScrollSpyViewportProbe::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<ScrollSpyViewportProbe, ScrollSpyViewportProbe::Extension>();
}

struct ScrollSpySectionProbe {
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  std::shared_ptr<detail::ScrollSpyControllerState> state;
  std::size_t index = 0;
  std::uint64_t token = 0;

  bool operator==(const ScrollSpySectionProbe&) const = default;
};

class ScrollSpySectionProbe::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const ScrollSpySectionProbe& value) { Update(node, value); }

  void Update(ViewNode&, const ScrollSpySectionProbe& value) {
    state_ = value.state;
    index_ = value.index;
    token_ = value.token;
  }

  PaintInvalidation PrepareGeometry(ViewNode& node, TextMeasurer&) override {
    if(!state_) return PaintInvalidation::None;
    const auto it = state_->sections.find(index_);
    if(it != state_->sections.end() && it->second.token == token_) {
      const Rect bounds = node.PresentationBounds();
      const bool valid = !bounds.IsEmpty();
      if(it->second.has_bounds != valid || it->second.window_bounds != bounds) {
        it->second.window_bounds = bounds;
        it->second.has_bounds = valid;
        ScheduleSync(*state_);
      }
    }
    return PaintInvalidation::None;
  }

private:
  std::shared_ptr<detail::ScrollSpyControllerState> state_;
  std::size_t index_ = 0;
  std::uint64_t token_ = 0;
};

const huxerui::detail::ModifierDescriptor& ScrollSpySectionProbe::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<ScrollSpySectionProbe, ScrollSpySectionProbe::Extension>();
}

} // namespace

std::size_t ScrollSpyController::ActiveSection() const {
  if(!state_ || !state_->active_section.IsValid()) return 0;
  return state_->active_section.Get();
}

std::size_t ScrollSpyController::SectionCount() const {
  if(!state_ || !state_->section_count.IsValid()) return 0;
  return state_->section_count.Get();
}

bool ScrollSpyController::ScrollToSection(std::size_t index) const {
  if(!state_ || !state_->has_viewport || !state_->style.scroll_controller.has_value()
     || !state_->style.scroll_controller->IsConnected()) {
    return false;
  }
  const auto it = state_->sections.find(index);
  if(it == state_->sections.end() || !it->second.has_bounds) {
    return false;
  }
  const bool vertical = state_->style.axis == Axis::Vertical;
  const float sec_start = vertical ? it->second.window_bounds.y : it->second.window_bounds.x;
  const float vp_start = vertical ? state_->viewport_bounds.y : state_->viewport_bounds.x;
  const float current_offset = state_->style.scroll_controller->Offset();
  const float target_offset = std::max(0.0F, current_offset + (sec_start - vp_start));
  return state_->style.scroll_controller->ScrollTo(target_offset);
}

ScrollSpyController UseScrollSpyController(std::size_t initial_section) {
  auto active_section = UseState(initial_section);
  auto section_count = UseState<std::size_t>(0);
  auto holder = UseState(std::make_shared<detail::ScrollSpyControllerState>());
  auto shared = holder.Get();
  shared->active_section = active_section;
  shared->section_count = section_count;
  return ScrollSpyController(std::move(shared));
}

View ScrollSpyArea(View content, ScrollSpyStyle style, std::function<void(std::size_t)> on_active_changed) {
  ValidateStyle(style);
  return Scope([content = std::move(content), style, on_active_changed = std::move(on_active_changed)]() -> View {
    TaskScope tasks = UseTaskScope();
    auto alive = UseState(std::make_shared<bool>(true)).Get();
    auto local_active = UseState<std::size_t>(0);
    auto local_count = UseState<std::size_t>(0);
    auto local_holder = UseState(std::make_shared<detail::ScrollSpyControllerState>());
    std::shared_ptr<detail::ScrollSpyControllerState> state =
        style.controller.state_ ? style.controller.state_ : local_holder.Get();
    if(!style.controller.state_) {
      state->active_section = local_active;
      state->section_count = local_count;
    }
    state->style = style;
    state->style.controller = {};
    state->on_active_changed = on_active_changed;
    state->alive = alive;
    *alive = true;

    std::weak_ptr<detail::ScrollSpyControllerState> weak_state = state;
    state->post_sync = [tasks, weak_state, alive]() {
      tasks.Post([weak_state, alive]() {
        if(!*alive) return;
        auto locked = weak_state.lock();
        if(!locked) return;
        locked->sync_scheduled = false;
        if(locked->section_count.IsValid() && locked->section_count.Get() != locked->sections.size()) {
          locked->section_count = locked->sections.size();
        }
        if(const std::optional<std::size_t> next = ComputeActiveSection(*locked); next.has_value()) {
          ApplyActiveSection(*locked, *next);
        }
      });
    };

    Lifecycle(
        [weak_state, alive]() {
          *alive = true;
          return [weak_state, alive]() {
            *alive = false;
            if(auto locked = weak_state.lock()) {
              locked->post_sync = {};
              locked->sync_scheduled = false;
            }
          };
        },
        state);

    View tracked = ProvideEnvironment(ScrollSpyEnvironment{state}, content);
    return Stack{std::move(tracked)}.With(ScrollSpyViewportProbe{
        .state = state,
        .axis = style.axis,
        .activation_offset = style.activation_offset,
    });
  });
}

View ScrollSpySection(View content, std::size_t index) {
  return Scope([content = std::move(content), index]() -> View {
    const auto& env = UseEnvironment<ScrollSpyEnvironment>();
    auto token_holder = UseState(std::make_shared<ScrollSpyTokenHolder>()).Get();
    View section = content;
    if(env.state) {
      if(token_holder->token == 0) {
        token_holder->token = env.state->next_token++;
      }
      const std::uint64_t token = token_holder->token;
      const auto existing = env.state->sections.find(index);
      if(existing == env.state->sections.end() || existing->second.token != token) {
        env.state->sections.insert_or_assign(index, detail::ScrollSpySectionEntry{.token = token});
      }
      std::weak_ptr<detail::ScrollSpyControllerState> weak_state = env.state;
      Lifecycle(
          [weak_state, index, token]() {
            return [weak_state, index, token]() {
              if(auto locked = weak_state.lock()) {
                const auto it = locked->sections.find(index);
                if(it != locked->sections.end() && it->second.token == token) {
                  locked->sections.erase(it);
                  locked->needs_sync = true;
                }
              }
            };
          },
          env.state, index, token);
      section = std::move(section).With(ScrollSpySectionProbe{
          .state = env.state,
          .index = index,
          .token = token,
      });
    }
    return section;
  });
}

} // namespace huxerui::interaction
