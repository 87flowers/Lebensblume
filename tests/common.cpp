#include <print>

#include "lb/assert.h"
#include "lb/common.h"

using namespace lb;

auto test_square() -> void {
  {
    const Square sq = Square::parse("1a").value();
    lb_assert(sq.raw == 0);
  }

  {
    const Square sq = Square::parse("9a").value();
    lb_assert(sq.raw == 8);
  }

  {
    const Square sq = Square::parse("9i").value();
    lb_assert(sq.raw == 80);
  }

  {
    lb_assert(!Square::parse("invalid").has_value());
  }

  // Roundtrip
  {
    for (u8 i = 0; i < 81; i++) {
      const Square sq{i};
      const std::string str = std::format("{}", sq);
      std::print("{} {}\n", i, sq);
      lb_assert(sq == Square::parse(str));
    }
  }
}

auto main() -> int {
  test_square();
  return 0;
}
