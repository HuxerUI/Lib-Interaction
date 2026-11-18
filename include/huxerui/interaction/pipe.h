#pragma once

#include <huxerui/view.h>

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>

namespace huxerui::interaction {

/// A composable View-to-View stage that chains from left to right with `operator|`.
class ViewAdaptor {
public:
  template <class Fn>
    requires(!std::same_as<std::remove_cvref_t<Fn>, ViewAdaptor>)
            && std::invocable<std::decay_t<Fn>&, View>
            && std::same_as<std::invoke_result_t<std::decay_t<Fn>&, View>, View>
  explicit ViewAdaptor(Fn&& fn) : apply_(std::forward<Fn>(fn)) {}

  [[nodiscard]] View operator()(View content) const {
    if(!apply_) return content;
    return apply_(std::move(content));
  }

private:
  std::function<View(View)> apply_;
};

/// Pipes `content` through `adaptor`, avoiding deeply nested wrapper calls.
/// Attach stateless decorators (DoubleClickable, ContextMenuArea, KeyboardShortcut) before stateful Scope-creating
/// wrappers (Pressable, HoverLift) when combining both onto one target node.
/// @code
/// View card = BaseCard()
///     .With(Ripple{})
///     | DoubleClickable([](const MultiTapEvent&) { ... })
///     | ContextMenuArea([](Point p) { ShowMenu(p); })
///     | Pressable([clicks] { clicks += 1; });
/// @endcode
[[nodiscard]] inline View operator|(View content, const ViewAdaptor& adaptor) {
  return adaptor(std::move(content));
}

} // namespace huxerui::interaction
