#include <huxerui/interaction/rollingnumber.h>

#include <huxerui/modifier.h>
#include <huxerui/semantics.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const RollingNumberStyle& style) {
  if(!std::isfinite(style.digit_gap) || style.digit_gap < 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: RollingNumber digit gap must be finite and non-negative");
  }
  if(style.minimum_digits < 0) {
    throw std::invalid_argument("HuxerUI Interaction: RollingNumber minimum digits must be non-negative");
  }
}

View DigitText(char digit, const RollingNumberStyle& style, float width, float height) {
  return Text(std::string(1, digit), TextRole::Body)
      .Style(style.text_style)
      .Align(TextAlign::Center)
      .VerticalAlign(TextVerticalAlign::Center)
      .With(Frame{.width = width, .height = height});
}

View DigitCell(int digit, const RollingNumberStyle& style, float width, float height) {
  std::vector<View> strip;
  strip.reserve(10);
  for(int value = 0; value < 10; ++value) {
    strip.push_back(DigitText(static_cast<char>('0' + value), style, width, height));
  }
  View track = Column(std::move(strip))
                   .With(Offset(AnimateTo(Point{0.0F, -static_cast<float>(digit) * height}, style.motion)));
  return Stack{std::move(track)}.With(Frame{.width = width, .height = height}, ClipChildren());
}

} // namespace

View RollingNumber(int value, RollingNumberStyle style) {
  ValidateStyle(style);
  return Scope([value, style]() -> View {
    TextMeasurer& measurer = UseTextMeasurer();
    std::array<float, 10> advances{};
    for(int digit = 0; digit < 10; ++digit) {
      advances[static_cast<std::size_t>(digit)] =
          measurer.MeasureRun(std::string(1, static_cast<char>('0' + digit)), style.text_style).advance;
    }
    const TextRunMetrics zero_metrics = measurer.MeasureRun("0", style.text_style);
    const float digit_width = std::max(1.0F, *std::max_element(advances.begin(), advances.end()));
    const float sign_width = std::max(1.0F, measurer.MeasureRun("-", style.text_style).advance);
    const float height = std::max(1.0F, zero_metrics.font_metrics.LineHeight());

    const long long magnitude = std::llabs(static_cast<long long>(value));
    std::string digits = std::to_string(magnitude);
    while(static_cast<int>(digits.size()) < style.minimum_digits) digits.insert(digits.begin(), '0');

    std::vector<View> cells;
    cells.reserve(digits.size() + (value < 0 ? 1 : 0));
    if(value < 0) {
      cells.push_back(Text("-", TextRole::Body)
                          .Style(style.text_style)
                          .Align(TextAlign::Center)
                          .VerticalAlign(TextVerticalAlign::Center)
                          .With(Frame{.width = sign_width, .height = height}));
    }
    for(char character : digits) {
      cells.push_back(DigitCell(character - '0', style, digit_width, height));
    }
    return Row(std::move(cells)).With(
        Spacing(style.digit_gap),
        Semantics{.role = SemanticRole::Text, .label = std::to_string(value),
                  .descendants = SemanticDescendantPolicy::Exclude});
  });
}

} // namespace huxerui::interaction
