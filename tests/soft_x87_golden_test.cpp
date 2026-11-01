#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include "math/backend.h"
#include "math/calculation.h"
#include "math/floating_environment.h"
#include "math/x87.h"

namespace {

void require(bool const condition, char const *const message) {
  if(!condition) throw std::runtime_error{message};
}

std::uint64_t bits_of(tfx::math::calculation const value) {
  return std::bit_cast<std::uint64_t>(static_cast<double>(value));
}

} // anonymous namespace

int main(int const argc, char const *const argv[]) try {
#if defined(TFX_MATH_HARDWARE_X87)
  // Hardware x87 transcendentals have their own rounding; soft goldens apply to the soft backend.
  (void)argc;(void)argv;
  return 77;
#endif
  if(argc != 2) throw std::runtime_error{"expected golden fixture path"};
  std::ifstream input{argv[1]};
  if(!input) throw std::runtime_error{"cannot open soft x87 golden fixture"};
  tfx::math::floating_environment const environment;
  std::string line;
  unsigned int checked{0};
  while(std::getline(input, line)) {
    if(line.empty() || line.front() == '#') continue;
    std::istringstream stream{line};
    std::string op;
    stream >> op;
    if(op == "sin") {
      double argument{0};
      std::uint64_t expected{0};
      stream >> argument >> expected;
      require(bits_of(tfx::math::x87::sin(tfx::math::calculation{argument})) == expected, "sin golden mismatch");
    } else if(op == "cos") {
      double argument{0};
      std::uint64_t expected{0};
      stream >> argument >> expected;
      require(bits_of(tfx::math::x87::cos(tfx::math::calculation{argument})) == expected, "cos golden mismatch");
    } else if(op == "atan2") {
      double y{0}, x{0};
      std::uint64_t expected{0};
      stream >> y >> x >> expected;
      require(bits_of(tfx::math::x87::atan2(tfx::math::calculation{y}, tfx::math::calculation{x})) == expected, "atan2 golden mismatch");
    } else throw std::runtime_error{"unknown golden op"};
    ++checked;
  }
  require(checked > 0, "no goldens executed");
  std::cout << "Soft x87 transcendental goldens matched (" << checked << ")\n";
  return 0;
} catch(std::exception const &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
