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

// Prefer calculation{1} / x over 1.0L / x. The latter divides in host long
// double before converting, which bypasses SoftFloat on non-x87 builds.

inline constexpr bool calculation_is_x87_precision =
  std::numeric_limits<calculation>::digits == 64;

} // namespace tfx::math
