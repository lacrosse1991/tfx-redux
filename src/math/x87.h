#pragma once

#include <cfloat>
#include <cmath>
#include "math/calculation.h"

namespace tfx::math::x87 {

#if defined(TFX_SOFT_X87)

// SoftFloat hosts keep Watcom's 53-bit arithmetic via floating_environment.
// On x86 verification builds, transcendentals still use the host instruction so
// SoftFloat arithmetic can be compared without inventing a second FSIN model.
// Non-x86 targets use libm until a soft FPATAN/FSIN/FCOS/FPTAN lands.
inline calculation atan2(calculation const y, calculation const x) {
#if (defined(__i386__) || defined(__x86_64__)) && defined(__GNUC__) && (LDBL_MANT_DIG == 64)
  long double result;
  long double const host_x{static_cast<long double>(x)};
  long double const host_y{static_cast<long double>(y)};
  __asm__("fpatan" : "=t"(result) : "0"(host_x), "u"(host_y) : "st(1)");
  return calculation{result};
#else
  return calculation{std::atan2(static_cast<double>(y), static_cast<double>(x))};
#endif
}

inline calculation sin(calculation const value) {
#if (defined(__i386__) || defined(__x86_64__)) && defined(__GNUC__) && (LDBL_MANT_DIG == 64)
  long double result;
  long double const host{static_cast<long double>(value)};
  __asm__("fsin" : "=t"(result) : "0"(host));
  return calculation{result};
#else
  return calculation{std::sin(static_cast<double>(value))};
#endif
}

inline calculation cos(calculation const value) {
#if (defined(__i386__) || defined(__x86_64__)) && defined(__GNUC__) && (LDBL_MANT_DIG == 64)
  long double result;
  long double const host{static_cast<long double>(value)};
  __asm__("fcos" : "=t"(result) : "0"(host));
  return calculation{result};
#else
  return calculation{std::cos(static_cast<double>(value))};
#endif
}

inline calculation tan(calculation const value) {
#if (defined(__i386__) || defined(__x86_64__)) && defined(__GNUC__) && (LDBL_MANT_DIG == 64)
  long double result;
  long double const host{static_cast<long double>(value)};
  __asm__("fptan; fstp %%st(0)" : "=t"(result) : "0"(host));
  return calculation{result};
#else
  return calculation{std::tan(static_cast<double>(value))};
#endif
}

#else

// The current native numerical backend already requires a 64-bit-significand
// long double. Keep the actual x87 transcendental rounding on x86 hosts.
inline long double atan2(long double const y, long double const x) {
#if (defined(__i386__) || defined(__x86_64__)) && defined(__GNUC__)
  long double result;
  __asm__("fpatan" : "=t"(result) : "0"(x), "u"(y) : "st(1)");
  return result;
#else
  return std::atan2(y,x);
#endif
}

inline long double sin(long double const value) {
#if (defined(__i386__) || defined(__x86_64__)) && defined(__GNUC__)
  long double result;
  __asm__("fsin" : "=t"(result) : "0"(value));
  return result;
#else
  return std::sin(value);
#endif
}
inline long double cos(long double const value) {
#if (defined(__i386__) || defined(__x86_64__)) && defined(__GNUC__)
  long double result;
  __asm__("fcos" : "=t"(result) : "0"(value));
  return result;
#else
  return std::cos(value);
#endif
}

inline long double tan(long double const value) {
#if (defined(__i386__) || defined(__x86_64__)) && defined(__GNUC__)
  long double result;
  __asm__("fptan; fstp %%st(0)" : "=t"(result) : "0"(value));
  return result;
#else
  return std::tan(value);
#endif
}

#endif

} // namespace tfx::math::x87
