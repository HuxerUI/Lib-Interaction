#pragma once

#include <huxerui/geometry.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

inline unsigned checks = 0;
#define CHECK(condition) do { ++checks; if(!(condition)) throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " #condition); } while(false)

template<class Exception, class F> void Throws(F&& action) {
  ++checks;
  try { action(); } catch(const Exception&) { return; }
  throw std::runtime_error("Expected exception was not thrown");
}

template <class F> int Run(F&& tests) {
  try { tests(); std::cout << checks << " checks passed\n"; return 0; }
  catch(const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}

inline huxerui::Point Center(const huxerui::Rect& bounds) {
  return {bounds.x + bounds.width * 0.5F, bounds.y + bounds.height * 0.5F};
}

inline bool Near(float actual, float expected) { return std::abs(actual - expected) <= 0.5F; }
