#pragma once

// Host arithmetic type for faithful flight intermediates.
// On x87 hosts this is long double (64-bit significand). Elsewhere — and when
// TFX_SOFT_X87 is forced for verification — it is the SoftFloat float80 wrapper.

#include <limits>
#include <numbers>

#if defined(TFX_SOFT_X87)
#include "math/float80.h"
#endif

namespace tfx::math {

#if defined(TFX_SOFT_X87)
using calculation = float80;
#else
using calculation = long double;
#endif

// Prefer calculation{1} / x over 1.0L / x. The latter divides in host long
// double before converting, which bypasses SoftFloat on non-x87 builds.

inline constexpr bool calculation_is_x87_precision =
  std::numeric_limits<calculation>::digits == 64;

// SoftFloat matches x87 width, but some hardware-recorded bit-exact fixtures
// still diverge on residual soft-trig paths. Gate those probes on the real
// x87 backend when a SoftFloat workaround is not yet in place.
inline constexpr bool calculation_is_hardware_x87 =
#if defined(TFX_SOFT_X87)
  false;
#else
  calculation_is_x87_precision;
#endif

// x87 FLDPI / long-double pi_v significand. On SoftFloat hosts, pi_v<long
// double> is only double-width and must not be used for extended half-pi.
inline calculation calculation_pi() {
#if defined(TFX_SOFT_X87)
  extFloat80_t bits{};
  bits.signExp = 0x4000;
  bits.signif = 0xc90fdaa22168c235ull;
  return calculation{bits};
#else
  return std::numbers::pi_v<long double>;
#endif
}

} // namespace tfx::math
