#include <huxerui/interaction/accessibility/rovingfocus.h>

#include <huxerui/environment.h>
#include <huxerui/lifecycle.h>
#include <huxerui/modifier.h>
#include <huxerui/state.h>

#include <cstdint>
#include <iterator>
#include <map>
#include <utility>

namespace huxerui::interaction {

namespace detail {

struct RovingFocusEntry {
  FocusRequester requester;
  std::uint64_t token = 0;
};

struct RovingFocusControllerState {
  State<std::size_t> active_index;
  RovingFocusStyle style;
  std::function<void(std::size_t)> on_active_changed;
  std::map<std::size_t, RovingFocusEntry> items;
  std::uint64_t next_token = 1;
};

} // namespace detail

namespace {

struct RovingFocusEnvironment {
  std::shared_ptr<detail::RovingFocusControllerState> state;

  static RovingFocusEnvironment Default() { return {}; }

  bool operator==(const RovingFocusEnvironment&) const = default;
};

struct RovingTokenHolder {
  std::uint64_t token = 0;
};

struct RovingFocusObserver {
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  std::shared_ptr<detail::RovingFocusControllerState> state;
  std::size_t index = 0;

  bool operator==(const RovingFocusObserver&) const = default;
};

void NotifyActiveIndex(detail::RovingFocusControllerState& state, std::size_t next_index) {
  if(state.active_index.IsValid()) {
    if(state.active_index.Get() == next_index) return;
    state.active_index = next_index;
  }
  if(state.on_active_changed) {
    state.on_active_changed(next_index);
  }
}

class RovingFocusObserver::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const RovingFocusObserver& value) { Update(node, value); }

  void Update(ViewNode&, const RovingFocusObserver& value) {
    state_ = value.state;
    index_ = value.index;
  }

  void OnFocusChanged(ViewNode&, bool focused, bool) override {
    if(focused && state_) {
      NotifyActiveIndex(*state_, index_);
    }
  }

private:
  std::shared_ptr<detail::RovingFocusControllerState> state_;
  std::size_t index_ = 0;
};

const huxerui::detail::ModifierDescriptor& RovingFocusObserver::Descriptor() {
  return huxerui::detail::ModifierDescriptorFor<RovingFocusObserver, RovingFocusObserver::Extension>();
}

bool FocusEndpoint(detail::RovingFocusControllerState& state, bool first) {
  if(state.items.empty()) return false;
  if(first) {
    for(const auto& [index, entry] : state.items) {
      if(entry.requester.RequestFocus()) {
        NotifyActiveIndex(state, index);
        return true;
      }
    }
  } else {
    for(auto it = state.items.rbegin(); it != state.items.rend(); ++it) {
      if(it->second.requester.RequestFocus()) {
        NotifyActiveIndex(state, it->first);
        return true;
      }
    }
  }
  return false;
}

bool FocusRelative(detail::RovingFocusControllerState& state, int direction) {
  if(state.items.empty()) return false;
  const std::size_t current = state.active_index.IsValid() ? state.active_index.Get() : 0;
  auto current_it = state.items.find(current);
  if(current_it == state.items.end()) {
    return FocusEndpoint(state, direction > 0);
  }
  const std::size_t total = state.items.size();
  auto cursor = current_it;
  for(std::size_t step = 0; step < total; ++step) {
    if(direction > 0) {
      ++cursor;
      if(cursor == state.items.end()) {
        if(!state.style.loop) return false;
        cursor = state.items.begin();
      }
    } else {
      if(cursor == state.items.begin()) {
        if(!state.style.loop) return false;
        cursor = std::prev(state.items.end());
      } else {
        --cursor;
      }
    }
    if(cursor == current_it) return false;
    if(cursor->second.requester.RequestFocus()) {
      NotifyActiveIndex(state, cursor->first);
      return true;
    }
  }
  return false;
}

} // namespace

std::size_t RovingFocusController::ActiveIndex() const {
  if(!state_ || !state_->active_index.IsValid()) return 0;
  return state_->active_index.Get();
}

bool RovingFocusController::FocusIndex(std::size_t index) const {
  if(!state_) return false;
  const auto it = state_->items.find(index);
  if(it == state_->items.end()) return false;
  if(!it->second.requester.RequestFocus()) return false;
  NotifyActiveIndex(*state_, index);
  return true;
}

bool RovingFocusController::FocusNext() const {
  return state_ ? FocusRelative(*state_, 1) : false;
}

bool RovingFocusController::FocusPrevious() const {
  return state_ ? FocusRelative(*state_, -1) : false;
}

bool RovingFocusController::FocusFirst() const {
  return state_ ? FocusEndpoint(*state_, true) : false;
}

bool RovingFocusController::FocusLast() const {
  return state_ ? FocusEndpoint(*state_, false) : false;
}

RovingFocusController UseRovingFocusController(std::size_t initial_index) {
  auto active_index = UseState(initial_index);
  auto holder = UseState(std::make_shared<detail::RovingFocusControllerState>());
  auto shared = holder.Get();
  shared->active_index = active_index;
  return RovingFocusController(std::move(shared));
}

View RovingFocusGroup(View content, RovingFocusStyle style, std::function<void(std::size_t)> on_active_changed) {
  return Scope([content = std::move(content), style, on_active_changed = std::move(on_active_changed)]() -> View {
    auto local_active = UseState<std::size_t>(0);
    auto local_holder = UseState(std::make_shared<detail::RovingFocusControllerState>());
    std::shared_ptr<detail::RovingFocusControllerState> state =
        style.controller.state_ ? style.controller.state_ : local_holder.Get();
    if(!style.controller.state_) {
      state->active_index = local_active;
    }
    state->style = style;
    state->style.controller = {};
    state->on_active_changed = on_active_changed;

    View group = ProvideEnvironment(RovingFocusEnvironment{state}, content);
    return Stack{std::move(group)}.On<ViewEvents::KeyIntercept>([state](const KeyEvent& event) -> bool {
      if(event.type != KeyEventType::Down) return false;
      if(event.modifiers.shift || event.modifiers.control || event.modifiers.alt || event.modifiers.meta) {
        return false;
      }
      const auto orientation = state->style.orientation;
      const bool allow_h =
          orientation == RovingFocusOrientation::Horizontal || orientation == RovingFocusOrientation::Both;
      const bool allow_v =
          orientation == RovingFocusOrientation::Vertical || orientation == RovingFocusOrientation::Both;
      switch(event.key) {
      case Key::ArrowRight:
        return allow_h && FocusRelative(*state, 1);
      case Key::ArrowLeft:
        return allow_h && FocusRelative(*state, -1);
      case Key::ArrowDown:
        return allow_v && FocusRelative(*state, 1);
      case Key::ArrowUp:
        return allow_v && FocusRelative(*state, -1);
      case Key::Home:
        return state->style.home_end && FocusEndpoint(*state, true);
      case Key::End:
        return state->style.home_end && FocusEndpoint(*state, false);
      default:
        return false;
      }
    });
  });
}

View RovingFocusItem(View content, std::size_t index, RovingFocusItemStyle style) {
  return Scope([content = std::move(content), index, style]() -> View {
    const auto& env = UseEnvironment<RovingFocusEnvironment>();
    FocusRequester requester = UseFocusRequester();
    auto token_holder = UseState(std::make_shared<RovingTokenHolder>()).Get();
    if(env.state) {
      if(token_holder->token == 0) {
        token_holder->token = env.state->next_token++;
      }
      const std::uint64_t token = token_holder->token;
      env.state->items.insert_or_assign(index, detail::RovingFocusEntry{requester, token});
      std::weak_ptr<detail::RovingFocusControllerState> state_weak = env.state;
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
    }

    View item = content;
    if(style.focusable) {
      item = std::move(item).With(Focusable{true});
    }
    item = std::move(item).With(FocusTarget{requester});
    if(env.state) {
      item = std::move(item).With(RovingFocusObserver{env.state, index});
    }
    return item;
  });
}

} // namespace huxerui::interaction
