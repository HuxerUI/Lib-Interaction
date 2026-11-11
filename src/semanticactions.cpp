#include <huxerui/interaction/semanticactions.h>

#include <stdexcept>
#include <utility>

namespace huxerui::interaction {
namespace {

constexpr std::uint64_t kStandardActionsMask =
    (std::uint64_t{1} << static_cast<std::uint8_t>(SemanticActionKind::Custom)) - 1U;

void ValidateActions(std::uint64_t actions) {
  if((actions & ~kStandardActionsMask) != 0) {
    throw std::invalid_argument(
        "HuxerUI Interaction: SemanticActions only supports standard SemanticActionKind values");
  }
}

} // namespace

class SemanticActions::Extension final : public NodeExtension {
public:
  Extension(ViewNode& node, const SemanticActions& value) { Update(node, value); }

  void Update(ViewNode&, const SemanticActions& value) {
    semantics_ = value.semantics;
    actions_ = value.actions;
    on_action_ = value.on_action;
  }

  void BuildSemantics(SemanticBuilder& builder) const override {
    builder.SetOwner(semantics_);
    for(std::uint8_t index = 0; index < static_cast<std::uint8_t>(SemanticActionKind::Custom); ++index) {
      const auto kind = static_cast<SemanticActionKind>(index);
      if((actions_ & SemanticActionMask(kind)) != 0) builder.AddAction(0, kind);
    }
  }

  [[nodiscard]] bool OnSemanticAction(std::uint64_t local_id, const SemanticAction& action) override {
    if(local_id != 0 || !on_action_) return false;
    if((actions_ & SemanticActionMask(action.kind)) == 0) return false;
    on_action_(action.kind);
    return true;
  }

private:
  Semantics semantics_;
  std::uint64_t actions_ = 0;
  std::function<void(SemanticActionKind)> on_action_;
};

const detail::ModifierDescriptor& SemanticActions::Descriptor() {
  return detail::ModifierDescriptorFor<SemanticActions, SemanticActions::Extension>();
}

SemanticActions::SemanticActions(Semantics semantics, std::uint64_t actions,
                                 std::function<void(SemanticActionKind)> on_action)
    : semantics(std::move(semantics)), actions(actions), on_action(std::move(on_action)) {
  ValidateActions(actions);
}

} // namespace huxerui::interaction
