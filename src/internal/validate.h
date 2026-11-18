#pragma once

#include <huxerui/geometry.h>

#include <chrono>
#include <cmath>
#include <optional>
#include <stdexcept>

namespace huxerui::interaction::internal {

inline void RequireFinite(float value, const char* message) {
  if(!std::isfinite(value)) {
    throw std::invalid_argument(message);
  }
}

inline void RequireFinite(double value, const char* message) {
  if(!std::isfinite(value)) {
    throw std::invalid_argument(message);
  }
}

inline void RequireFinite(Point value, const char* message) {
  if(!std::isfinite(value.x) || !std::isfinite(value.y)) {
    throw std::invalid_argument(message);
  }
}

inline void RequirePositive(float value, const char* message) {
  if(!std::isfinite(value) || value <= 0.0F) {
    throw std::invalid_argument(message);
  }
}

inline void RequirePositive(double value, const char* message) {
  if(!std::isfinite(value) || value <= 0.0) {
    throw std::invalid_argument(message);
  }
}

inline void RequireNonNegative(float value, const char* message) {
  if(!std::isfinite(value) || value < 0.0F) {
    throw std::invalid_argument(message);
  }
}

inline void RequireNonNegative(double value, const char* message) {
  if(!std::isfinite(value) || value < 0.0) {
    throw std::invalid_argument(message);
  }
}

inline void RequireUnitInterval(float value, const char* message) {
  if(!std::isfinite(value) || value < 0.0F || value > 1.0F) {
    throw std::invalid_argument(message);
  }
}

inline void RequirePositiveUnitInterval(float value, const char* message) {
  if(!std::isfinite(value) || value <= 0.0F || value > 1.0F) {
    throw std::invalid_argument(message);
  }
}

inline void RequirePositiveDuration(std::chrono::duration<double> value, const char* message) {
  RequirePositive(value.count(), message);
}

inline void RequireNonNegativeDuration(std::chrono::duration<double> value, const char* message) {
  RequireNonNegative(value.count(), message);
}

inline void RequireOptionalPositiveDuration(const std::optional<std::chrono::duration<double>>& value,
                                            const char* message) {
  if(value.has_value()) {
    RequirePositiveDuration(*value, message);
  }
}

inline void RequireOptionalPositive(const std::optional<float>& value, const char* message) {
  if(value.has_value()) {
    RequirePositive(*value, message);
  }
}

inline void RequireScaleBounds(float minimum_scale, float maximum_scale, const char* min_message,
                               const char* max_message, const char* order_message) {
  RequirePositive(minimum_scale, min_message);
  RequirePositive(maximum_scale, max_message);
  if(minimum_scale > maximum_scale) {
    throw std::invalid_argument(order_message);
  }
}

} // namespace huxerui::interaction::internal
