#pragma once

#include "math/backend.h"
#include "math/soft_control.h"
#include <stdexcept>

namespace tfx::math {

// Watcom's original FPU detection/reset (0xde851 -> 0xea0fc) loads 0x127f:
// masked exceptions, nearest rounding and a 53-bit arithmetic significand.
// x87 transcendental results still retain their instruction's extended format.
class floating_environment {
private:
#if defined(TFX_MATH_HARDWARE_X87)
  unsigned short previous{0};
  bool changed{false};
#endif

public:
  floating_environment() {
#if defined(TFX_MATH_HARDWARE_X87)
    unsigned short constexpr native_control{native_fpu_control};
    __asm__ volatile("fnstcw %0" : "=m"(previous));
    changed = previous != native_control;
    if(changed) __asm__ volatile("fldcw %0" : : "m"(native_control) : "memory");
#elif defined(TFX_MATH_SOFT_X87)
    soft_x87_detail::push_native_control();
#else
    throw std::runtime_error{"native numerical flight currently requires an x87 arithmetic backend"};
#endif
  }
  ~floating_environment() {
#if defined(TFX_MATH_HARDWARE_X87)
    if(changed) __asm__ volatile("fldcw %0" : : "m"(previous) : "memory");
#elif defined(TFX_MATH_SOFT_X87)
    soft_x87_detail::pop_control();
#endif
  }
  floating_environment(floating_environment const &) = delete;
  floating_environment &operator=(floating_environment const &) = delete;
};

} // namespace tfx::math
