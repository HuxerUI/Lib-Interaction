#include <huxerui/interaction/press/copyable.h>

#include "../internal/validate.h"

#include <huxerui/app.h>
#include <huxerui/clipboard.h>
#include <huxerui/modifier.h>
#include <huxerui/state.h>
#include <huxerui/task.h>

#include <cstdint>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const CopyableStyle& style) {
  internal::RequirePositiveDuration(
      style.feedback_duration, "HuxerUI Interaction: Copyable feedback duration must be finite and positive");
}

} // namespace

View Copyable(std::function<View(bool copied)> builder, std::string text,
              std::function<void(bool succeeded)> on_copied, CopyableStyle style) {
  ValidateStyle(style);
  if(!builder) {
    throw std::invalid_argument("HuxerUI Interaction: Copyable requires a view builder");
  }

  return Scope([builder = std::move(builder), text = std::move(text), on_copied = std::move(on_copied), style]() -> View {
    const auto clipboard = UseApplication().Clipboard();
    TaskScope tasks = UseTaskScope();
    auto copied = UseState(false);
    auto generation = UseState<std::uint64_t>(0);
    const bool available = clipboard && clipboard->IsAvailable();

    View surface = builder(copied.Get());
    surface = std::move(surface).OnClick([clipboard, tasks, copied, generation, text, style, on_copied] {
      const bool succeeded = clipboard && clipboard->WriteText(text);
      if(succeeded) {
        const std::uint64_t token = generation.Get() + 1U;
        generation = token;
        copied = true;
        tasks.Launch([generation, token, copied, duration = style.feedback_duration]() -> Task<void> {
          co_await Delay(duration);
          if(generation.Get() != token) co_return;
          copied = false;
        });
      }
      if(on_copied) on_copied(succeeded);
    });
    if(style.cursor != PointerCursorKind::Default) {
      surface = std::move(surface).With(PointerCursor{style.cursor});
    }
    if(style.disable_when_unavailable) {
      surface = std::move(surface).With(Enabled{available});
    }
    return surface;
  });
}

View Copyable(std::function<View(bool copied)> builder, std::string text, CopyableStyle style) {
  return Copyable(std::move(builder), std::move(text), std::function<void(bool)>{}, style);
}

View Copyable(View content, std::string text, std::function<void(bool succeeded)> on_copied, CopyableStyle style) {
  return Copyable([content = std::move(content)](bool) -> View { return content; }, std::move(text),
                  std::move(on_copied), style);
}

View Copyable(View content, std::string text, CopyableStyle style) {
  return Copyable(std::move(content), std::move(text), std::function<void(bool)>{}, style);
}

} // namespace huxerui::interaction
