#pragma once

#include "math/backend.h"
#include <cstdint>

namespace tfx::math {

// Soft mirror of the Watcom startup control word 0x127f: masked exceptions,
// round-to-nearest, and a 53-bit arithmetic significand (PC field).
inline constexpr std::uint16_t native_fpu_control{0x127f};

#if defined(TFX_MATH_SOFT_X87)

namespace soft_x87_detail {

inline thread_local std::uint16_t control_word{native_fpu_control};
inline thread_local std::uint16_t previous_control{native_fpu_control};
inline thread_local unsigned depth{0};

inline std::uint16_t current_control() { return control_word; }

inline void push_native_control() {
  if(depth++ == 0) previous_control = control_word;
  control_word = native_fpu_control;
}

inline void pop_control() {
  if(depth == 0) return;
  if(--depth == 0) control_word = previous_control;
}

// PC field in the x87 control word: bits 8-9. 0x127f uses 0b10 = 53-bit.
inline unsigned precision_bits(std::uint16_t const control = current_control()) {
  switch((control >> 8) & 0x3u) {
  case 0: return 24;
  case 2: return 53;
  default: return 64;
  }
}

} // namespace soft_x87_detail

#endif

} // namespace tfx::math
