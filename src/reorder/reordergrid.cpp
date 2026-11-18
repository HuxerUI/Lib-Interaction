#include <huxerui/interaction/reorder/reordergrid.h>

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

void ValidateStyle(const ReorderableGridStyle& style) {
  internal::RequirePositive(style.cell_width,
                            "HuxerUI Interaction: ReorderableGrid cell width must be finite and positive");
  internal::RequirePositive(style.cell_height,
                            "HuxerUI Interaction: ReorderableGrid cell height must be finite and positive");
  internal::RequireNonNegative(style.spacing,
                               "HuxerUI Interaction: ReorderableGrid spacing must be finite and non-negative");
  internal::RequireNonNegative(
      style.target_highlight_width,
      "HuxerUI Interaction: ReorderableGrid highlight width must be finite and non-negative");
  internal::RequireOptionalPositive(
      style.auto_scroll_edge, "HuxerUI Interaction: ReorderableGrid auto-scroll edge must be finite and positive");
  internal::RequirePositive(style.auto_scroll_step,
                            "HuxerUI Interaction: ReorderableGrid auto-scroll step must be finite and positive");
}

} // namespace

View ReorderableGridImpl(std::vector<View> items, std::function<void(std::size_t, std::size_t)> on_reordered,
                         ReorderableGridStyle style) {
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
    const Frame cell_frame{.width = style.cell_width, .height = style.cell_height};
    std::vector<View> slots;
    slots.reserve(count);
    for(std::size_t index = 0; index < count; ++index) {
      const Point offset = internal::ComputeSlotOffset(session, *shared_ctx->geometry, index);
      const AnimationSpec& motion =
          (session.active && index == session.from) ? style.follow_motion : style.settle_motion;

      View item = items[index];
      item = std::move(item).With(cell_frame, Offset(AnimateTo(offset, motion)));

      View slot = Stack{std::move(item)}.With(cell_frame, internal::SlotBounds{shared_ctx->geometry, index});
      if(!style.handle_only) {
        slot = internal::BindReorderDrag(std::move(slot), shared_ctx, index);
      }
      if(style.keyboard_reorder) {
        slot = internal::BindKeyboardReorder(std::move(slot), shared_ctx, index, Axis::Horizontal, true);
      }
      slot = std::move(slot).With(Semantics{
          .role = SemanticRole::GridCell,
          .collection_item = SemanticCollectionItem{.index = index},
      });
      if(session.active && session.target == index && session.from != index) {
        slot = std::move(slot).With(Border{.color = style.target_highlight, .width = style.target_highlight_width});
      }
      slots.push_back(ProvideEnvironment(
          internal::ReorderSlotEnvironment{.context = shared_ctx, .index = index}, std::move(slot)));
    }
    return Flow(std::move(slots))
        .With(Spacing(style.spacing), internal::RootBounds{shared_ctx->root_bounds},
              Semantics{.role = SemanticRole::Grid, .collection = SemanticCollection{.item_count = count}});
  });
}

} // namespace huxerui::interaction::detail
