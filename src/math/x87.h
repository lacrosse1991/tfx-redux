#pragma once

#include "math/backend.h"
#include "math/calculation.h"

namespace tfx::math::x87 {

// Hardware path keeps real x87 transcendental rounding. Soft path uses the
// in-tree extended implementation under a 64-bit significand result format.
inline calculation atan2(calculation const y, calculation const x) {
#if defined(TFX_MATH_HARDWARE_X87)
  long double result;
  __asm__("fpatan" : "=t"(result) : "0"(static_cast<long double>(x)), "u"(static_cast<long double>(y)) : "st(1)");
  return result;
#else
  return soft_atan2(y, x);
#endif
}

inline calculation sin(calculation const value) {
#if defined(TFX_MATH_HARDWARE_X87)
  long double result;
  __asm__("fsin" : "=t"(result) : "0"(static_cast<long double>(value)));
  return result;
#else
  return soft_sin(value);
#endif
}

inline calculation cos(calculation const value) {
#if defined(TFX_MATH_HARDWARE_X87)
  long double result;
  __asm__("fcos" : "=t"(result) : "0"(static_cast<long double>(value)));
  return result;
#else
  return soft_cos(value);
#endif
}

inline calculation tan(calculation const value) {
#if defined(TFX_MATH_HARDWARE_X87)
  long double result;
  __asm__("fptan; fstp %%st(0)" : "=t"(result) : "0"(static_cast<long double>(value)));
  return result;
#else
  return soft_tan(value);
#endif
}

} // namespace tfx::math::x87
