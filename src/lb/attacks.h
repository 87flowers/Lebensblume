#pragma once

#include <ranges>
#include <utility>

#include "lb/common.h"

namespace lb::attacks {
  constexpr auto allPawns(Bitboard pieces, Color piece_color) -> Bitboard { return pieces.shiftRelative(Direction::n, piece_color); }

  constexpr auto allKnights(Bitboard pieces, Color piece_color) -> Bitboard {
    Bitboard result{};
    result |= pieces.shiftRelative(Direction::n, piece_color).shiftRelative(Direction::nw, piece_color);
    result |= pieces.shiftRelative(Direction::n, piece_color).shiftRelative(Direction::ne, piece_color);
    return result;
  }

  constexpr auto allSilvers(Bitboard pieces, Color piece_color) -> Bitboard {
    Bitboard result{};
    result |= pieces.shiftRelative(Direction::nw, piece_color);
    result |= pieces.shiftRelative(Direction::n, piece_color);
    result |= pieces.shiftRelative(Direction::ne, piece_color);
    result |= pieces.shiftRelative(Direction::sw, piece_color);
    result |= pieces.shiftRelative(Direction::se, piece_color);
    return result;
  }

  constexpr auto allGolds(Bitboard pieces, Color piece_color) -> Bitboard {
    Bitboard result{};
    result |= pieces.shiftRelative(Direction::nw, piece_color);
    result |= pieces.shiftRelative(Direction::n, piece_color);
    result |= pieces.shiftRelative(Direction::ne, piece_color);
    result |= pieces.shiftRelative(Direction::w, piece_color);
    result |= pieces.shiftRelative(Direction::e, piece_color);
    result |= pieces.shiftRelative(Direction::s, piece_color);
    return result;
  }

  constexpr auto allKings(Bitboard pieces, Color piece_color) -> Bitboard {
    Bitboard result{};
    result |= pieces.shift(Direction::n);
    result |= pieces.shift(Direction::ne);
    result |= pieces.shift(Direction::e);
    result |= pieces.shift(Direction::se);
    result |= pieces.shift(Direction::s);
    result |= pieces.shift(Direction::sw);
    result |= pieces.shift(Direction::w);
    result |= pieces.shift(Direction::nw);
    return result;
  }

  namespace table {

    consteval auto generateRelative(auto f) -> std::array<std::array<Bitboard, 81>, 2> {
      std::array<std::array<Bitboard, 81>, 2> result;
      for (Color c : {Color::sente, Color::gote})
        for (u8 i : std::views::iota(0, 81))
          result[std::to_underlying(c)][i] = f(Bitboard::fromSq(Square{i}), c);
      return result;
    }

    consteval auto generate(auto f) -> std::array<Bitboard, 81> {
      std::array<Bitboard, 81> result;
      for (u8 i : std::views::iota(0, 81))
        result[i] = f(Bitboard::fromSq(Square{i}));
      return result;
    }

    inline constexpr pawn = generateRelative(allPawns);
    inline constexpr knight = generateRelative(allKnights);
    inline constexpr silver = generateRelative(allSilvers);
    inline constexpr gold = generateRelative(allGolds);
    inline constexpr king = generate(allKings);

  } // namespace table

  inline constexpr auto pawn(Square sq, Color piece_color) -> Bitboard { return table::pawn[std::to_underlying(piece_color)][sq.raw]; }
  inline constexpr auto knight(Square sq, Color piece_color) -> Bitboard { return table::knight[std::to_underlying(piece_color)][sq.raw]; }
  inline constexpr auto silver(Square sq, Color piece_color) -> Bitboard { return table::silver[std::to_underlying(piece_color)][sq.raw]; }
  inline constexpr auto gold(Square sq, Color piece_color) -> Bitboard { return table::gold[std::to_underlying(piece_color)][sq.raw]; }
  inline constexpr auto king(Square sq) -> Bitboard { return table::king[sq.raw]; }

} // namespace lb::attacks
