#include <bit>
#include <iostream>
#include <stdexcept>
#include "math/backend.h"
#include "math/calculation.h"
#include "math/floating_environment.h"
#include "math/x87.h"

// On hardware hosts, rebuild with -DTFX_NUMERICAL_BACKEND=soft and compare those
// outputs against a hardware build. This smoke tool verifies the active backend
// produces finite sin/cos/atan2 results under the native control word.

namespace {

void require(bool const condition, char const *const message) {
  if(!condition) throw std::runtime_error{message};
}

} // anonymous namespace

int main() try {
  tfx::math::floating_environment const environment;
  auto const s{tfx::math::x87::sin(tfx::math::calculation{0.5})};
  auto const c{tfx::math::x87::cos(tfx::math::calculation{0.5})};
  auto const a{tfx::math::x87::atan2(tfx::math::calculation{0.5}, tfx::math::calculation{0.75})};
  require(tfx::math::isfinite(s) && tfx::math::isfinite(c) && tfx::math::isfinite(a), "transcendental produced non-finite");
  std::cout << "backend "
#if defined(TFX_MATH_SOFT_X87)
    << "soft"
#else
    << "hardware"
#endif
    << " sin=" << std::bit_cast<std::uint64_t>(static_cast<double>(s))
    << " cos=" << std::bit_cast<std::uint64_t>(static_cast<double>(c))
    << " atan2=" << std::bit_cast<std::uint64_t>(static_cast<double>(a)) << '\n';
  return 0;
} catch(std::exception const &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
