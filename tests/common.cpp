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
      lb_assert(sq == Square::parse(str));
    }
  }
}

auto test_move() -> void {
  lb_assert(std::format("{}", Move::makeDrop(PieceType::bishop, Square::parse("3f").value())) == "B*3f");
  lb_assert(std::format("{}", Move::makeDrop(PieceType::rook, Square::parse("7i").value())) == "R*7i");
  lb_assert(std::format("{}", Move::makeMove(Square::parse("2e").value(), Square::parse("7e").value(), false)) == "2e7e");
  lb_assert(std::format("{}", Move::makeMove(Square::parse("5f").value(), Square::parse("5a").value(), true)) == "5f5a+");
}

auto main() -> int {
  test_square();
  test_move();
  return 0;
}
