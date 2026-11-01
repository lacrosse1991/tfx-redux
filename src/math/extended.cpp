#include "math/extended.h"

#if defined(TFX_MATH_SOFT_X87)

#include <bit>
#include <cstring>
#include <utility>

namespace tfx::math {
namespace {

constexpr std::uint16_t exp_bias{0x3fff};
constexpr std::uint16_t exp_max{0x7fff};
constexpr std::uint64_t integer_bit{0x8000000000000000ull};

struct uint128 {
  std::uint64_t lo{};
  std::uint64_t hi{};
};

uint128 mul_u64(std::uint64_t const a, std::uint64_t const b) {
  std::uint64_t const a_lo{a & 0xffffffffull}, a_hi{a >> 32};
  std::uint64_t const b_lo{b & 0xffffffffull}, b_hi{b >> 32};
  std::uint64_t const p0{a_lo * b_lo};
  std::uint64_t const p1{a_lo * b_hi};
  std::uint64_t const p2{a_hi * b_lo};
  std::uint64_t const p3{a_hi * b_hi};
  std::uint64_t const mid{(p0 >> 32) + (p1 & 0xffffffffull) + (p2 & 0xffffffffull)};
  return {(p0 & 0xffffffffull) | (mid << 32), p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32)};
}

uint128 add_u128(uint128 a, uint128 const b) {
  std::uint64_t const lo{a.lo + b.lo};
  a.hi += b.hi + (lo < a.lo ? 1u : 0u);
  a.lo = lo;
  return a;
}

uint128 sub_u128(uint128 a, uint128 const b) {
  std::uint64_t const lo{a.lo - b.lo};
  a.hi -= b.hi + (a.lo < b.lo ? 1u : 0u);
  a.lo = lo;
  return a;
}

int cmp_u128(uint128 const a, uint128 const b) {
  if(a.hi != b.hi) return a.hi < b.hi ? -1 : 1;
  if(a.lo == b.lo) return 0;
  return a.lo < b.lo ? -1 : 1;
}

uint128 shl1(uint128 value) {
  value.hi = (value.hi << 1) | (value.lo >> 63);
  value.lo <<= 1;
  return value;
}

struct divmod_result {
  uint128 quot;
  std::uint64_t rem;
};

divmod_result divmod_128_64(uint128 const num, std::uint64_t const den) {
  uint128 rem{};
  uint128 quot{};
  for(int bit{127}; bit >= 0; --bit) {
    rem = shl1(rem);
    rem.lo |= (bit >= 64 ? (num.hi >> (bit - 64)) : (num.lo >> bit)) & 1ull;
    if(rem.hi != 0 || rem.lo >= den) {
      rem = sub_u128(rem, {den, 0});
      if(bit >= 64) quot.hi |= 1ull << (bit - 64);
      else quot.lo |= 1ull << bit;
    }
  }
  return {quot, rem.lo};
}

struct unpacked {
  bool sign{};
  int exponent{};
  std::uint64_t significand{};
  bool infinite{};
  bool nan{};
  bool zero{};
};

unpacked unpack(extended const value) {
  unpacked result;
  result.sign = (value.sign_exp & 0x8000u) != 0;
  std::uint16_t const biased{static_cast<std::uint16_t>(value.sign_exp & 0x7fffu)};
  result.significand = value.significand;
  if(biased == exp_max) {
    result.infinite = (value.significand << 1) == 0;
    result.nan = !result.infinite;
    return result;
  }
  if(biased == 0 && value.significand == 0) {
    result.zero = true;
    return result;
  }
  result.exponent = static_cast<int>(biased) - static_cast<int>(exp_bias);
  if(biased == 0) {
    unsigned const shift{static_cast<unsigned>(std::countl_zero(value.significand))};
    result.significand <<= shift;
    result.exponent = 1 - static_cast<int>(exp_bias) - static_cast<int>(shift);
  }
  return result;
}

extended pack(bool const sign, int exponent, std::uint64_t significand, unsigned const precision) {
  if(significand == 0) return {0, static_cast<std::uint16_t>(sign ? 0x8000u : 0u)};
  if((significand & integer_bit) == 0) {
    unsigned const shift{static_cast<unsigned>(std::countl_zero(significand))};
    significand <<= shift;
    exponent -= static_cast<int>(shift);
  }
  if(precision < 64) {
    unsigned const drop{64u - precision};
    std::uint64_t const half{1ull << (drop - 1u)};
    std::uint64_t const mask{(~0ull) << drop};
    std::uint64_t const remainder{significand & ((1ull << drop) - 1ull)};
    significand &= mask;
    bool const odd{((significand >> drop) & 1ull) != 0};
    if(remainder > half || (remainder == half && odd)) {
      std::uint64_t const increment{1ull << drop};
      std::uint64_t const summed{significand + increment};
      if(summed < significand) {
        significand = integer_bit;
        ++exponent;
      } else significand = summed;
    }
  }
  if(exponent > static_cast<int>(exp_bias)) {
    return {integer_bit, static_cast<std::uint16_t>((sign ? 0x8000u : 0u) | exp_max)};
  }
  int const biased{exponent + static_cast<int>(exp_bias)};
  if(biased <= 0) {
    unsigned const shift{static_cast<unsigned>(1 - biased)};
    if(shift >= 64) return {0, static_cast<std::uint16_t>(sign ? 0x8000u : 0u)};
    std::uint64_t denorm{significand >> shift};
    if(shift != 0 && ((significand >> (shift - 1u)) & 1ull) != 0) ++denorm;
    if(denorm == 0) return {0, static_cast<std::uint16_t>(sign ? 0x8000u : 0u)};
    return {denorm, static_cast<std::uint16_t>(sign ? 0x8000u : 0u)};
  }
  return {significand, static_cast<std::uint16_t>((sign ? 0x8000u : 0u) | static_cast<std::uint16_t>(biased))};
}

extended pack_current(bool const sign, int const exponent, std::uint64_t const significand) {
  return pack(sign, exponent, significand, soft_x87_detail::precision_bits());
}

struct precision_scope {
  std::uint16_t previous;
  explicit precision_scope(unsigned const bits) : previous{soft_x87_detail::control_word} {
    unsigned field{3};
    if(bits == 24) field = 0;
    else if(bits == 53) field = 2;
    soft_x87_detail::control_word = static_cast<std::uint16_t>((previous & ~0x300u) | (field << 8));
  }
  ~precision_scope() { soft_x87_detail::control_word = previous; }
};

extended from_ieee754(bool const sign, int const unbiased_exp, std::uint64_t const frac, int const frac_bits, int const bias) {
  int const all_ones{(bias * 2) + 1};
  int const biased{unbiased_exp + bias};
  if(biased == all_ones) {
    if(frac == 0) return {integer_bit, static_cast<std::uint16_t>((sign ? 0x8000u : 0u) | exp_max)};
    return {integer_bit | (frac << (63 - frac_bits)), static_cast<std::uint16_t>((sign ? 0x8000u : 0u) | exp_max)};
  }
  if(biased == 0 && frac == 0) return {0, static_cast<std::uint16_t>(sign ? 0x8000u : 0u)};
  if(biased == 0) {
    unsigned const leading{static_cast<unsigned>(std::countl_zero(frac))};
    unsigned const width{64u - static_cast<unsigned>(frac_bits)};
    unsigned const shift{leading - width};
    std::uint64_t const sig{frac << (shift + (64u - static_cast<unsigned>(frac_bits)))};
    return pack(sign, 1 - bias - static_cast<int>(shift), sig, 64);
  }
  return pack(sign, unbiased_exp, integer_bit | (frac << (63 - frac_bits)), 64);
}

int cmp_magnitudes(unpacked const &a, unpacked const &b) {
  if(a.infinite && b.infinite) return 0;
  if(a.infinite) return 1;
  if(b.infinite) return -1;
  if(a.zero && b.zero) return 0;
  if(a.zero) return -1;
  if(b.zero) return 1;
  if(a.exponent != b.exponent) return a.exponent < b.exponent ? -1 : 1;
  if(a.significand == b.significand) return 0;
  return a.significand < b.significand ? -1 : 1;
}

extended mul_full(extended const a, extended const b) {
  precision_scope const scope{64};
  return a * b;
}
extended add_full(extended const a, extended const b) {
  precision_scope const scope{64};
  return a + b;
}
extended div_full(extended const a, extended const b) {
  precision_scope const scope{64};
  return a / b;
}

extended poly_sin(extended const x) {
  extended const x2{mul_full(x, x)};
  extended p{extended{-1.9572941063392612e-4}};
  p = add_full(mul_full(p, x2), extended{8.333329304842563e-3});
  p = add_full(mul_full(p, x2), extended{-1.6666666665957816e-1});
  return add_full(x, mul_full(mul_full(x, x2), p));
}

extended poly_cos(extended const x) {
  extended const x2{mul_full(x, x)};
  extended p{extended{2.439044879627741e-5}};
  p = add_full(mul_full(p, x2), extended{-1.388888887449255e-3});
  p = add_full(mul_full(p, x2), extended{4.166666666622608e-2});
  p = add_full(mul_full(p, x2), extended{-0.5});
  return add_full(extended{1.0}, mul_full(x2, p));
}

extended poly_atan(extended const x) {
  extended const x2{mul_full(x, x)};
  extended p{extended{0.016691405295293}};
  p = add_full(mul_full(p, x2), extended{-0.058518165875937});
  p = add_full(mul_full(p, x2), extended{0.089764049046704});
  p = add_full(mul_full(p, x2), extended{-0.111107873967942});
  p = add_full(mul_full(p, x2), extended{0.142857135618071});
  p = add_full(mul_full(p, x2), extended{-0.199999999963173});
  p = add_full(mul_full(p, x2), extended{0.333333333333317});
  return add_full(x, mul_full(mul_full(x, x2), p));
}

extended reduce_pi_over_two(extended value, int &quadrant) {
  extended const two_pi{mul_full(extended_pi, extended{2.0})};
  // Subtract only whole turns; a fractional multiple of 2π cancels the argument.
  double const turns{std::nearbyint(static_cast<double>(value) / 6.28318530717958647692)};
  if(turns != 0.0) value = value - mul_full(extended{turns}, two_pi);
  if(value > extended_pi) value = value - two_pi;
  if(value < -extended_pi) value = value + two_pi;
  quadrant = 0;
  if(value > extended_half_pi) {
    value = extended_pi - value;
    quadrant = 1;
  } else if(value < -extended_half_pi) {
    value = -extended_pi - value;
    quadrant = -1;
  }
  return value;
}

} // namespace

extended::extended(float const value) noexcept {
  std::uint32_t bits{};
  std::memcpy(&bits, &value, sizeof(bits));
  *this = from_ieee754((bits >> 31) != 0, static_cast<int>((bits >> 23) & 0xffu) - 127, bits & 0x7fffffu, 23, 127);
}

extended::extended(double const value) noexcept {
  std::uint64_t bits{};
  std::memcpy(&bits, &value, sizeof(bits));
  *this = from_ieee754((bits >> 63) != 0, static_cast<int>((bits >> 52) & 0x7ffu) - 1023, bits & 0xfffffffffffffu, 52, 1023);
}

extended::extended(long double const value) noexcept {
  *this = extended{static_cast<double>(value)};
}

extended::operator float() const noexcept { return static_cast<float>(static_cast<double>(*this)); }

extended::operator double() const noexcept {
  auto const u{unpack(*this)};
  if(u.nan) return std::numeric_limits<double>::quiet_NaN();
  if(u.infinite) return std::copysign(std::numeric_limits<double>::infinity(), u.sign ? -1.0 : 1.0);
  if(u.zero) return std::copysign(0.0, u.sign ? -1.0 : 1.0);
  int exp{u.exponent};
  std::uint64_t const shift_out{u.significand & ((1ull << 11) - 1ull)};
  std::uint64_t mant{(u.significand >> 11) & 0xfffffffffffffu};
  if(shift_out > (1ull << 10) || (shift_out == (1ull << 10) && (mant & 1ull) != 0)) {
    ++mant;
    if(mant == (1ull << 52)) {
      mant = 0;
      ++exp;
    }
  }
  if(exp > 1023) return std::copysign(std::numeric_limits<double>::infinity(), u.sign ? -1.0 : 1.0);
  if(exp < -1022) return std::copysign(0.0, u.sign ? -1.0 : 1.0);
  std::uint64_t bits{(u.sign ? 1ull : 0ull) << 63};
  bits |= static_cast<std::uint64_t>(exp + 1023) << 52;
  bits |= mant;
  double result;
  std::memcpy(&result, &bits, sizeof(result));
  return result;
}

extended::operator long double() const noexcept { return static_cast<long double>(static_cast<double>(*this)); }

bool operator==(extended const a, extended const b) noexcept {
  auto const ua{unpack(a)}, ub{unpack(b)};
  if(ua.nan || ub.nan) return false;
  if(ua.zero && ub.zero) return true;
  return a.sign_exp == b.sign_exp && a.significand == b.significand;
}

bool operator<(extended const a, extended const b) noexcept {
  auto const ua{unpack(a)}, ub{unpack(b)};
  if(ua.nan || ub.nan) return false;
  if(ua.zero && ub.zero) return false;
  if(ua.sign != ub.sign) return ua.sign;
  int const mag{cmp_magnitudes(ua, ub)};
  return ua.sign ? mag > 0 : mag < 0;
}

extended operator+(extended a, extended b) noexcept {
  auto ua{unpack(a)}, ub{unpack(b)};
  if(ua.nan) return a;
  if(ub.nan) return b;
  if(ua.infinite && ub.infinite && ua.sign != ub.sign) return std::numeric_limits<extended>::quiet_NaN();
  if(ua.infinite) return a;
  if(ub.infinite) return b;
  if(ua.zero) return b;
  if(ub.zero) return a;
  if(ua.exponent < ub.exponent || (ua.exponent == ub.exponent && ua.significand < ub.significand)) std::swap(ua, ub);
  unsigned const shift{static_cast<unsigned>(ua.exponent - ub.exponent)};
  uint128 wide_a{ua.significand, 0};
  uint128 wide_b{shift >= 64 ? 0ull : (ub.significand >> shift), 0};
  uint128 sum;
  if(ua.sign == ub.sign) sum = add_u128(wide_a, wide_b);
  else {
    sum = sub_u128(wide_a, wide_b);
    if(sum.lo == 0 && sum.hi == 0) return {0, 0};
  }
  int exp{ua.exponent};
  std::uint64_t sig;
  if(sum.hi != 0) {
    sig = (sum.hi << 63) | (sum.lo >> 1); // only low bit of hi should be set for 65-bit sum
    if(sum.hi > 1) {
      // Normalise from higher overflow (shouldn't happen for 64+64 add of normalised)
      sig = sum.hi;
    } else {
      sig = (sum.lo >> 1) | (sum.hi << 63);
      if(sum.lo & 1ull) sig |= 1ull;
    }
    ++exp;
  } else sig = sum.lo;
  return pack_current(ua.sign, exp, sig);
}

extended operator-(extended const a, extended const b) noexcept {
  extended neg{b};
  neg.sign_exp = static_cast<std::uint16_t>(neg.sign_exp ^ 0x8000u);
  return a + neg;
}

extended operator*(extended const a, extended const b) noexcept {
  auto const ua{unpack(a)}, ub{unpack(b)};
  bool const sign{ua.sign != ub.sign};
  if(ua.nan) return a;
  if(ub.nan) return b;
  if((ua.infinite && ub.zero) || (ub.infinite && ua.zero)) return std::numeric_limits<extended>::quiet_NaN();
  if(ua.infinite || ub.infinite) return {integer_bit, static_cast<std::uint16_t>((sign ? 0x8000u : 0u) | exp_max)};
  if(ua.zero || ub.zero) return {0, static_cast<std::uint16_t>(sign ? 0x8000u : 0u)};
  uint128 const product{mul_u64(ua.significand, ub.significand)};
  int exp{ua.exponent + ub.exponent};
  std::uint64_t sig;
  if((product.hi & integer_bit) != 0) {
    sig = product.hi;
    if(product.lo != 0) sig |= 1ull;
    ++exp;
  } else {
    sig = (product.hi << 1) | (product.lo >> 63);
    if((product.lo << 1) != 0) sig |= 1ull;
  }
  return pack_current(sign, exp, sig);
}

extended operator/(extended const a, extended const b) noexcept {
  auto const ua{unpack(a)}, ub{unpack(b)};
  bool const sign{ua.sign != ub.sign};
  if(ua.nan) return a;
  if(ub.nan) return b;
  if((ua.infinite && ub.infinite) || (ua.zero && ub.zero)) return std::numeric_limits<extended>::quiet_NaN();
  if(ua.infinite || ub.zero) return {integer_bit, static_cast<std::uint16_t>((sign ? 0x8000u : 0u) | exp_max)};
  if(ub.infinite || ua.zero) return {0, static_cast<std::uint16_t>(sign ? 0x8000u : 0u)};
  // quot = (a_sig << 64) / b_sig = (a_sig / b_sig) * 2^64, with a/b in (0.5, 2).
  auto const divided{divmod_128_64({0, ua.significand}, ub.significand)};
  int exp{ua.exponent - ub.exponent};
  std::uint64_t sig;
  if(divided.quot.hi != 0) {
    // quot in [2^64, 2^65): right-shift into a normalised 64-bit significand.
    sig = (divided.quot.lo >> 1) | (divided.quot.hi << 63);
    if((divided.quot.lo & 1ull) != 0 || divided.rem != 0) sig |= 1ull;
  } else {
    sig = divided.quot.lo;
    if(divided.rem != 0) sig |= 1ull;
    // quot in [2^63, 2^64): already normalised when a_sig >= b_sig / 2.
    // If the integer bit is clear, normalise left (ratio < 1).
    if((sig & integer_bit) == 0 && sig != 0) {
      unsigned const shift{static_cast<unsigned>(std::countl_zero(sig))};
      sig <<= shift;
      exp -= static_cast<int>(shift);
    }
  }
  return pack_current(sign, exp, sig);
}

extended sqrt(extended const value) noexcept {
  auto const u{unpack(value)};
  if(u.nan || u.zero || u.infinite) return value;
  if(u.sign) return std::numeric_limits<extended>::quiet_NaN();
  extended x{std::sqrt(static_cast<double>(value))};
  precision_scope const scope{64};
  for(int i{0}; i != 4; ++i) x = (x + value / x) * extended{0.5};
  return pack_current((x.sign_exp & 0x8000u) != 0,
    static_cast<int>(x.sign_exp & 0x7fffu) - static_cast<int>(exp_bias),
    x.significand);
}

extended soft_sin(extended const value) noexcept {
  precision_scope const scope{64};
  auto const u{unpack(value)};
  if(u.zero) return {0, static_cast<std::uint16_t>(u.sign ? 0x8000u : 0u)};
  int quadrant{0};
  extended const x{reduce_pi_over_two(value, quadrant)};
  if(quadrant == 0) return poly_sin(x);
  if(quadrant > 0) return poly_cos(x);
  return -poly_cos(x);
}

extended soft_cos(extended const value) noexcept {
  precision_scope const scope{64};
  int quadrant{0};
  extended const x{reduce_pi_over_two(value, quadrant)};
  if(quadrant == 0) return poly_cos(x);
  if(quadrant > 0) return -poly_sin(x);
  return poly_sin(x);
}

extended soft_tan(extended const value) noexcept {
  precision_scope const scope{64};
  return div_full(soft_sin(value), soft_cos(value));
}

extended soft_atan2(extended const y, extended const x) noexcept {
  precision_scope const scope{64};
  auto const uy{unpack(y)}, ux{unpack(x)};
  if(uy.nan) return y;
  if(ux.nan) return x;
  if(uy.zero && ux.zero) {
    if(!ux.sign) return y;
    return uy.sign ? -extended_pi : extended_pi;
  }
  if(ux.zero) return uy.sign ? -extended_half_pi : extended_half_pi;
  if(uy.zero) return ux.sign ? (uy.sign ? -extended_pi : extended_pi) : y;
  if(ux.infinite && uy.infinite) {
    extended const eighth{div_full(extended_pi, extended{4.0})};
    extended result{ux.sign ? mul_full(eighth, extended{3.0}) : eighth};
    return uy.sign ? -result : result;
  }
  if(ux.infinite) return ux.sign ? (uy.sign ? -extended_pi : extended_pi) : (uy.sign ? -extended{0.0} : extended{0.0});
  if(uy.infinite) return uy.sign ? -extended_half_pi : extended_half_pi;

  extended const ay{abs(y)}, ax{abs(x)};
  bool swapped{false};
  extended ratio;
  if(ay > ax) {
    ratio = div_full(ax, ay);
    swapped = true;
  } else ratio = div_full(ay, ax);
  extended angle;
  if(ratio > extended{0.5}) {
    // atan(x) = pi/4 + atan((x-1)/(x+1)) keeps the polynomial near zero.
    extended const t{div_full(ratio - extended{1.0}, ratio + extended{1.0})};
    angle = add_full(div_full(extended_pi, extended{4.0}), poly_atan(t));
  } else angle = poly_atan(ratio);
  if(swapped) angle = extended_half_pi - angle;
  if(ux.sign) angle = extended_pi - angle;
  return uy.sign ? -angle : angle;
}

} // namespace tfx::math

#endif
