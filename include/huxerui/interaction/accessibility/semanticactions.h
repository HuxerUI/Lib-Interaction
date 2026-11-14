#pragma once

#include <huxerui/modifier.h>
#include <huxerui/semantics.h>

#include <cstdint>
#include <functional>

namespace huxerui::interaction {

/// Adds explicit standard accessibility actions to a view and routes them to a callback.
///
/// `Semantics` already declares role, label, value, and state. This modifier declares the standard actions that the
/// extension handles through `OnSemanticAction`; the callback receives the requested kind after UiWindow validates
/// availability and payload shape. Custom actions are not represented; use a dedicated NodeExtension when a local
/// custom action ID is required.
/// @code
/// Text("Delete")
///     .With(Semantics{.role = SemanticRole::Button, .label = "Delete"})
///     .With(SemanticActions{Semantics{}, SemanticActionMask(SemanticActionKind::Activate),
///                           [](SemanticActionKind) { Delete(); }});
/// @endcode
class SemanticActions {
public:
  /// Retained extension created by the framework when the modifier is attached through With().
  class Extension;

  /// Returns the retained-modifier descriptor used by View::With().
  static const detail::ModifierDescriptor& Descriptor();

  /// Creates a standard-action contribution.
  /// @param semantics Owner semantic properties merged with other contributions.
  /// @param actions Bitmask of standard `SemanticActionKind` actions. Custom is rejected.
  /// @param on_action Callback invoked for a requested advertised action; an empty function leaves actions
  ///     advertised but unhandled, which is normally not useful.
  /// @throws std::invalid_argument If the mask contains Custom or an invalid bit.
  SemanticActions(Semantics semantics, std::uint64_t actions,
                  std::function<void(SemanticActionKind)> on_action = {});

  /// Owner semantic properties.
  Semantics semantics;
  /// Advertised standard action mask.
  std::uint64_t actions = 0;
  /// Action handler.
  std::function<void(SemanticActionKind)> on_action;
};

} // namespace huxerui::interaction
