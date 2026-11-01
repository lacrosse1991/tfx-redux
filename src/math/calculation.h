#pragma once

#include "math/backend.h"
#include "math/extended.h"
#include <cmath>

namespace tfx::math {

#if defined(TFX_MATH_HARDWARE_X87)
using calculation = long double;

inline calculation sqrt(calculation const value) noexcept { return std::sqrt(value); }
inline calculation abs(calculation const value) noexcept { return std::fabs(value); }
inline bool isfinite(calculation const value) noexcept { return std::isfinite(value); }
inline calculation pow(calculation const base, calculation const exponent) noexcept { return std::pow(base, exponent); }
inline calculation trunc(calculation const value) noexcept { return std::trunc(value); }

inline calculation half_pi() noexcept { return calculation{3.14159265358979323846L} / 2; }
#else
using calculation = extended;

inline calculation half_pi() noexcept { return extended_half_pi; }
inline calculation pow(calculation const base, calculation const exponent) noexcept {
  return extended{std::pow(static_cast<double>(base), static_cast<double>(exponent))};
}
inline calculation trunc(calculation const value) noexcept {
  return extended{std::trunc(static_cast<double>(value))};
}
#endif

inline bool isfinite(float const value) noexcept { return std::isfinite(value); }
inline bool isfinite(double const value) noexcept { return std::isfinite(value); }
inline bool isfinite(long double const value) noexcept { return std::isfinite(value); }

} // namespace tfx::math

