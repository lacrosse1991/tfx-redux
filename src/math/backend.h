#pragma once

#include <cfloat>

// Compile-time selection between host x87 and the soft extended backend.
// CMake may force TFX_NUMERICAL_BACKEND_SOFT or TFX_NUMERICAL_BACKEND_HARDWARE.
#if defined(TFX_NUMERICAL_BACKEND_SOFT)
#  define TFX_MATH_SOFT_X87 1
#elif defined(TFX_NUMERICAL_BACKEND_HARDWARE)
#  define TFX_MATH_HARDWARE_X87 1
#elif (defined(__i386__) || defined(__x86_64__)) && defined(__GNUC__) && (LDBL_MANT_DIG == 64)
#  define TFX_MATH_HARDWARE_X87 1
#else
#  define TFX_MATH_SOFT_X87 1
#endif

#if defined(TFX_MATH_HARDWARE_X87) && defined(TFX_MATH_SOFT_X87)
#  error "conflicting TFX numerical backend selection"
#endif
