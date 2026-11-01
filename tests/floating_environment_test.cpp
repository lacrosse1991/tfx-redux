#include <iostream>
#include <stdexcept>
#include "math/backend.h"
#include "math/calculation.h"
#include "math/floating_environment.h"
#include "math/soft_control.h"

namespace {

#if defined(TFX_MATH_HARDWARE_X87)
unsigned short control_word() {
  unsigned short result;
  __asm__ volatile("fnstcw %0" : "=m"(result));
  return result;
}
#else
std::uint16_t control_word() { return tfx::math::soft_x87_detail::current_control(); }
#endif

void require(bool const condition, char const *const message) {
  if(!condition) throw std::runtime_error{message};
}

} // anonymous namespace

int main() try {
  auto const previous{control_word()};
  {
    tfx::math::floating_environment const environment;
    require(control_word() == tfx::math::native_fpu_control,"native precision/rounding control differs");
#if defined(TFX_MATH_HARDWARE_X87)
    volatile long double one{1.0L}, half_ulp{0x1p-53L};
    long double const rounded{one + half_ulp};
    require(rounded == 1.0L,"native arithmetic does not round at 53 bits");
#else
    tfx::math::calculation const one{1.0};
    tfx::math::calculation const half_ulp{0x1p-53};
    tfx::math::calculation const rounded{one + half_ulp};
    require(rounded == one,"soft arithmetic does not round at 53 bits");
#endif
    {
      tfx::math::floating_environment const nested;
      require(control_word() == tfx::math::native_fpu_control,"nested precision scope differs");
    }
    require(control_word() == tfx::math::native_fpu_control,"nested scope changed the outer precision");
  }
  require(control_word() == previous,"native scope leaked its control word to the host");
  try {
    tfx::math::floating_environment const environment;
    throw 1;
  } catch(int) {}
  require(control_word() == previous,"exception did not restore host precision");
  std::cout << "Native arithmetic precision, nested scopes and exception restoration agree\n";
  return 0;
} catch(std::exception const &error) { std::cerr << error.what() << '\n'; return 1; }
