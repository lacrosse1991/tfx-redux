#pragma once

#include "math/backend.h"
#include "math/soft_control.h"
#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace tfx::math {

#if defined(TFX_MATH_SOFT_X87)

// Software IEEE 8087 extended-precision value: 1 sign + 15 exp + 64 significand.
class extended {
public:
  std::uint64_t significand{};
  std::uint16_t sign_exp{};

  constexpr extended() noexcept = default;
  constexpr extended(std::uint64_t const sig, std::uint16_t const se) noexcept : significand{sig}, sign_exp{se} {}

  extended(float const value) noexcept;
  extended(double const value) noexcept;
  extended(long double const value) noexcept;
  extended(bool) = delete;
  template<class Int, std::enable_if_t<std::is_integral_v<Int> && !std::is_same_v<Int,bool>, int> = 0>
  extended(Int const value) noexcept : extended{static_cast<long double>(value)} {}

  explicit operator float() const noexcept;
  explicit operator double() const noexcept;
  explicit operator long double() const noexcept;
  template<class Int, std::enable_if_t<std::is_integral_v<Int> && !std::is_same_v<Int,bool>, int> = 0>
  explicit operator Int() const noexcept {
    return static_cast<Int>(static_cast<long double>(*this));
  }

  friend extended operator+(extended a, extended b) noexcept;
  friend extended operator-(extended a, extended b) noexcept;
  friend extended operator*(extended a, extended b) noexcept;
  friend extended operator/(extended a, extended b) noexcept;
  friend extended operator+(extended a) noexcept { return a; }
  friend extended operator-(extended a) noexcept {
    a.sign_exp = static_cast<std::uint16_t>(a.sign_exp ^ 0x8000u);
    return a;
  }

  extended &operator+=(extended const b) noexcept { return *this = *this + b; }
  extended &operator-=(extended const b) noexcept { return *this = *this - b; }
  extended &operator*=(extended const b) noexcept { return *this = *this * b; }
  extended &operator/=(extended const b) noexcept { return *this = *this / b; }

  friend bool operator==(extended a, extended b) noexcept;
  friend bool operator!=(extended a, extended b) noexcept { return !(a == b); }
  friend bool operator<(extended a, extended b) noexcept;
  friend bool operator>(extended a, extended b) noexcept { return b < a; }
  friend bool operator<=(extended a, extended b) noexcept { return !(b < a); }
  friend bool operator>=(extended a, extended b) noexcept { return !(a < b); }
};

extended sqrt(extended value) noexcept;
extended abs(extended value) noexcept;
bool isfinite(extended value) noexcept;
bool isnan(extended value) noexcept;

extended soft_atan2(extended y, extended x) noexcept;
extended soft_sin(extended value) noexcept;
extended soft_cos(extended value) noexcept;
extended soft_tan(extended value) noexcept;

inline constexpr extended extended_pi{0xc90fdaa22168c235ull, 0x4000u}; // pi
inline constexpr extended extended_half_pi{0xc90fdaa22168c235ull, 0x3fffu};

#else

using extended = long double;

inline extended soft_atan2(extended const y, extended const x) noexcept { return std::atan2(y, x); }
inline extended soft_sin(extended const value) noexcept { return std::sin(value); }
inline extended soft_cos(extended const value) noexcept { return std::cos(value); }
inline extended soft_tan(extended const value) noexcept { return std::tan(value); }

#endif

} // namespace tfx::math

#if defined(TFX_MATH_SOFT_X87)

namespace std {

template<>
class numeric_limits<tfx::math::extended> {
public:
  static constexpr bool is_specialized{true};
  static constexpr bool is_signed{true};
  static constexpr bool is_integer{false};
  static constexpr bool is_exact{false};
  static constexpr bool has_infinity{true};
  static constexpr bool has_quiet_NaN{true};
  static constexpr bool has_denorm{true};
  static constexpr int digits{64};
  static constexpr int digits10{18};
  static constexpr int max_digits10{21};
  static constexpr int radix{2};
  static constexpr int min_exponent{-16381};
  static constexpr int max_exponent{16384};
  static constexpr bool is_iec559{false};
  static constexpr bool is_bounded{true};
  static constexpr bool traps{false};
  static constexpr bool tinyness_before{false};
  static constexpr float_round_style round_style{round_to_nearest};

  static constexpr tfx::math::extended min() noexcept { return {0x8000000000000000ull, 1u}; }
  static constexpr tfx::math::extended lowest() noexcept { return {0x8000000000000000ull, 0x8001u}; }
  static constexpr tfx::math::extended max() noexcept { return {0xffffffffffffffffull, 0x7ffeu}; }
  static constexpr tfx::math::extended epsilon() noexcept { return {0x8000000000000000ull, 0x3fc0u}; }
  static constexpr tfx::math::extended infinity() noexcept { return {0x8000000000000000ull, 0x7fffu}; }
  static constexpr tfx::math::extended quiet_NaN() noexcept { return {0xc000000000000000ull, 0x7fffu}; }
  static constexpr tfx::math::extended denorm_min() noexcept { return {1ull, 0u}; }
};

template<>
inline constexpr bool is_floating_point_v<tfx::math::extended> = true;

template<>
struct is_floating_point<tfx::math::extended> : true_type {};

} // namespace std

namespace tfx::math {

inline extended abs(extended value) noexcept {
  value.sign_exp = static_cast<std::uint16_t>(value.sign_exp & 0x7fffu);
  return value;
}

inline bool isnan(extended const value) noexcept {
  return (value.sign_exp & 0x7fffu) == 0x7fffu && (value.significand << 1) != 0;
}

inline bool isfinite(extended const value) noexcept {
  return (value.sign_exp & 0x7fffu) != 0x7fffu;
}

} // namespace tfx::math

#endif
