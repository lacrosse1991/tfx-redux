#pragma once

// Host arithmetic type for faithful flight intermediates.
// On x87 hosts this is long double (64-bit significand). Elsewhere — and when
// TFX_SOFT_X87 is forced for verification — it is the SoftFloat float80 wrapper.

#include <limits>

#if defined(TFX_SOFT_X87)
#include "math/float80.h"
#endif

namespace tfx::math {

#if defined(TFX_SOFT_X87)
using calculation = float80;
#else
using calculation = long double;
#endif

inline constexpr bool calculation_is_x87_precision =
  std::numeric_limits<calculation>::digits == 64;

// SoftFloat matches x87 width, but some hardware-recorded bit-exact fixtures
// still diverge. Gate those probes on the real x87 backend.
inline constexpr bool calculation_is_hardware_x87 =
#if defined(TFX_SOFT_X87)
  false;
#else
  calculation_is_x87_precision;
#endif

} // namespace tfx::math
