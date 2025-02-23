#include <print>

#include "lb/common.h"
#include "lb/util/assert.h"

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

auto test_shift() -> void {
  {
    const Bitboard base{0x800};
    lb_assert(base.shift(Direction::n).raw == 0x000004);
    lb_assert(base.shift(Direction::ne).raw == 0x000002);
    lb_assert(base.shift(Direction::e).raw == 0x000400);
    lb_assert(base.shift(Direction::se).raw == 0x080000);
    lb_assert(base.shift(Direction::s).raw == 0x100000);
    lb_assert(base.shift(Direction::sw).raw == 0x200000);
    lb_assert(base.shift(Direction::w).raw == 0x001000);
    lb_assert(base.shift(Direction::nw).raw == 0x000008);
  }

  {
    const Bitboard base = Bitboard::fromSq(Square::parse("7e").value());
    lb_assert(base.shift(Direction::n).toSq() == Square::parse("7d"));
    lb_assert(base.shift(Direction::ne).toSq() == Square::parse("6d"));
    lb_assert(base.shift(Direction::e).toSq() == Square::parse("6e"));
    lb_assert(base.shift(Direction::se).toSq() == Square::parse("6f"));
    lb_assert(base.shift(Direction::s).toSq() == Square::parse("7f"));
    lb_assert(base.shift(Direction::sw).toSq() == Square::parse("8f"));
    lb_assert(base.shift(Direction::w).toSq() == Square::parse("8e"));
    lb_assert(base.shift(Direction::nw).toSq() == Square::parse("8d"));
  }

  {
    const Bitboard base = Bitboard::fromSq(Square::parse("7e").value());
    lb_assert(base.shiftRelative(Direction::n, Color::sente).toSq() == Square::parse("7d"));
    lb_assert(base.shiftRelative(Direction::ne, Color::sente).toSq() == Square::parse("6d"));
    lb_assert(base.shiftRelative(Direction::e, Color::sente).toSq() == Square::parse("6e"));
    lb_assert(base.shiftRelative(Direction::se, Color::sente).toSq() == Square::parse("6f"));
    lb_assert(base.shiftRelative(Direction::s, Color::sente).toSq() == Square::parse("7f"));
    lb_assert(base.shiftRelative(Direction::sw, Color::sente).toSq() == Square::parse("8f"));
    lb_assert(base.shiftRelative(Direction::w, Color::sente).toSq() == Square::parse("8e"));
    lb_assert(base.shiftRelative(Direction::nw, Color::sente).toSq() == Square::parse("8d"));
  }

  {
    const Bitboard base = Bitboard::fromSq(Square::parse("7e").value());
    lb_assert(base.shiftRelative(Direction::n, Color::gote).toSq() == Square::parse("7f"));
    lb_assert(base.shiftRelative(Direction::ne, Color::gote).toSq() == Square::parse("8f"));
    lb_assert(base.shiftRelative(Direction::e, Color::gote).toSq() == Square::parse("8e"));
    lb_assert(base.shiftRelative(Direction::se, Color::gote).toSq() == Square::parse("8d"));
    lb_assert(base.shiftRelative(Direction::s, Color::gote).toSq() == Square::parse("7d"));
    lb_assert(base.shiftRelative(Direction::sw, Color::gote).toSq() == Square::parse("6d"));
    lb_assert(base.shiftRelative(Direction::w, Color::gote).toSq() == Square::parse("6e"));
    lb_assert(base.shiftRelative(Direction::nw, Color::gote).toSq() == Square::parse("6f"));
  }
}

auto main() -> int {
  test_square();
  test_move();
  test_shift();
  return 0;
}
