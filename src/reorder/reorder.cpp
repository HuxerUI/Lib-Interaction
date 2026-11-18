#include <huxerui/interaction/reorder/reorder.h>

#include "../internal/reorder_core.h"
#include "../internal/validate.h"

#include <huxerui/environment.h>
#include <huxerui/modifier.h>
#include <huxerui/semantics.h>
#include <huxerui/state.h>

#include <memory>
#include <utility>

namespace huxerui::interaction::detail {
namespace {

void ValidateStyle(const ReorderableStyle& style) {
  internal::RequireNonNegative(style.spacing,
                               "HuxerUI Interaction: Reorderable spacing must be finite and non-negative");
  internal::RequireNonNegative(style.target_highlight_width,
                               "HuxerUI Interaction: Reorderable highlight width must be finite and non-negative");
  internal::RequireOptionalPositive(style.auto_scroll_edge,
                                    "HuxerUI Interaction: Reorderable auto-scroll edge must be finite and positive");
  internal::RequirePositive(style.auto_scroll_step,
                            "HuxerUI Interaction: Reorderable auto-scroll step must be finite and positive");
}

} // namespace

View ReorderableImpl(std::vector<View> items, std::function<void(std::size_t, std::size_t)> on_reordered,
                     ReorderableStyle style) {
  ValidateStyle(style);
  return Scope([items = std::move(items), style, on_reordered = std::move(on_reordered)]() -> View {
    auto drag = UseState(internal::ReorderSession{});
    auto ctx_holder = UseState(std::make_shared<internal::ReorderContext>());
    const auto shared_ctx = ctx_holder.Get();
    const std::size_t count = items.size();

    shared_ctx->drag = drag;
    shared_ctx->count = count;
    shared_ctx->auto_scroll_edge = style.auto_scroll_edge;
    shared_ctx->auto_scroll_step = style.auto_scroll_step;
    shared_ctx->scroll_controller = style.scroll_controller;
    shared_ctx->on_reordered = on_reordered;

    const internal::ReorderSession session = drag.Get();
    std::vector<View> slots;
    slots.reserve(count);
    for(std::size_t index = 0; index < count; ++index) {
      const Point offset = internal::ComputeSlotOffset(session, *shared_ctx->geometry, index);
      const AnimationSpec& motion =
          (session.active && index == session.from) ? style.follow_motion : style.settle_motion;

      View item = items[index];
      item = std::move(item).With(Offset(AnimateTo(offset, motion)));

      View slot = Column{std::move(item)}.With(internal::SlotBounds{shared_ctx->geometry, index});
      if(!style.handle_only) {
        slot = internal::BindReorderDrag(std::move(slot), shared_ctx, index);
      }
      if(style.keyboard_reorder) {
        slot = internal::BindKeyboardReorder(std::move(slot), shared_ctx, index, style.axis, false);
      }
      slot = std::move(slot).With(Semantics{
          .role = SemanticRole::ListItem,
          .collection_item = SemanticCollectionItem{.index = index},
      });
      if(session.active && session.target == index && session.from != index) {
        slot = std::move(slot).With(Border{.color = style.target_highlight, .width = style.target_highlight_width});
      }
      slots.push_back(ProvideEnvironment(
          internal::ReorderSlotEnvironment{.context = shared_ctx, .index = index}, std::move(slot)));
    }
    View list = style.axis == Axis::Vertical ? View(Column(std::move(slots))) : View(Row(std::move(slots)));
    return std::move(list).With(
        Spacing(style.spacing), internal::RootBounds{shared_ctx->root_bounds},
        Semantics{.role = SemanticRole::List, .collection = SemanticCollection{.item_count = count}});
  });
}

} // namespace huxerui::interaction::detail
