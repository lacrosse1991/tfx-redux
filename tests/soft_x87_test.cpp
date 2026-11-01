#include <cmath>
#include <iostream>
#include <stdexcept>
#include "math/backend.h"
#include "math/calculation.h"
#include "math/floating_environment.h"
#include "math/x87.h"

namespace {

void require(bool const condition, char const *const message) {
  if(!condition) throw std::runtime_error{message};
}

double as_double(tfx::math::calculation const value) { return static_cast<double>(value); }

} // anonymous namespace

int main() try {
  {
    tfx::math::floating_environment const environment;
    tfx::math::calculation const one{1.0};
    tfx::math::calculation const half_ulp{0x1p-53};
    require((one + half_ulp) == one, "PC=53 addition did not round away a half-ulp");
    require((one * tfx::math::calculation{2.0}) == tfx::math::calculation{2.0}, "multiplication identity failed");
    require(as_double(tfx::math::sqrt(tfx::math::calculation{4.0})) == 2.0, "sqrt(4) failed");
  }
  {
    tfx::math::floating_environment const environment;
    auto const s{tfx::math::x87::sin(tfx::math::calculation{0.0})};
    auto const c{tfx::math::x87::cos(tfx::math::calculation{0.0})};
    require(as_double(s) == 0.0, "sin(0) failed");
    require(std::abs(as_double(c) - 1.0) < 1e-12, "cos(0) failed");
    auto const a{tfx::math::x87::atan2(tfx::math::calculation{1.0}, tfx::math::calculation{1.0})};
    require(std::abs(as_double(a) - 0.7853981633974483) < 1e-9, "atan2(1,1) failed");
  }
  std::cout << "Soft/hardware calculation backend arithmetic and transcendentals passed\n";
  return 0;
} catch(std::exception const &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
