#pragma once

#include <ranges>
#include <utility>

#include "lb/bit.h"
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

  constexpr auto allKings(Bitboard pieces) -> Bitboard {
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

    inline constexpr std::array<std::array<Bitboard, 81>, 2> pawn = generateRelative(allPawns);
    inline constexpr std::array<std::array<Bitboard, 81>, 2> knight = generateRelative(allKnights);
    inline constexpr std::array<std::array<Bitboard, 81>, 2> silver = generateRelative(allSilvers);
    inline constexpr std::array<std::array<Bitboard, 81>, 2> gold = generateRelative(allGolds);
    inline constexpr std::array<Bitboard, 81> king = generate(allKings);

  } // namespace table

  inline constexpr auto pawn(Square sq, Color piece_color) -> Bitboard { return table::pawn[std::to_underlying(piece_color)][sq.raw]; }
  inline constexpr auto knight(Square sq, Color piece_color) -> Bitboard { return table::knight[std::to_underlying(piece_color)][sq.raw]; }
  inline constexpr auto silver(Square sq, Color piece_color) -> Bitboard { return table::silver[std::to_underlying(piece_color)][sq.raw]; }
  inline constexpr auto gold(Square sq, Color piece_color) -> Bitboard { return table::gold[std::to_underlying(piece_color)][sq.raw]; }
  inline constexpr auto king(Square sq) -> Bitboard { return table::king[sq.raw]; }

  namespace sliders {

    struct SliderTable {
      Bitboard mask;
      u64 compressed_mask;
      const Bitboard *ptr;
    };

    inline constexpr auto compressBlockers(Bitboard bb) -> u64 { return static_cast<u64>(bb.raw) | ((bb.raw >> 64) << 1); }
    inline constexpr auto decompressBlockers(u64 y, Bitboard mask) -> Bitboard {
      return Bitboard{(y & mask.raw) | ((static_cast<u128>(y >> 1) << 64) & mask.raw)};
    }

    using BlockerArray = std::array<Bitboard, 81>;

    inline consteval auto generatePotentialBlockers(Bitboard piece, auto directions) -> Bitboard {
      Bitboard result{};
      for (Direction dir : directions)
        for (Bitboard current = piece.shift(dir); !current.shift(dir).empty(); current = current.shift(dir))
          result |= current;
      return result;
    }

    inline consteval auto generatePotentialBlockers(auto directions) -> BlockerArray {
      BlockerArray result;
      for (u8 i : std::views::iota(0, 81)) {
        const Bitboard piece = Bitboard::fromSq(Square{i});
        result[i] = generatePotentialBlockers(piece, directions);
      }
      return result;
    }

    inline consteval auto generateAttackCount(const BlockerArray &blocker_array) -> usize {
      usize result = 0;
      for (Bitboard b : blocker_array) {
        const int count = std::popcount(compressBlockers(b));
        result += 1 << count;
      }
      return result;
    }

    constexpr std::array<Direction, 4> bishop_dirs = {Direction::ne, Direction::se, Direction::sw, Direction::nw};
    constexpr std::array<Direction, 4> rook_dirs = {Direction::n, Direction::e, Direction::s, Direction::w};
    constexpr std::array<Direction, 1> lance_sente_dirs = {Direction::n};
    constexpr std::array<Direction, 1> lance_gote_dirs = {Direction::s};

    extern const std::array<SliderTable, 81> bishop_lut;
    extern const std::array<SliderTable, 81> rook_lut;
    extern const std::array<std::array<SliderTable, 81>, 2> lance_lut;

  } // namespace sliders

  inline constexpr auto bishop(Square sq, Bitboard blockers) -> Bitboard {
    const sliders::SliderTable &t = sliders::bishop_lut[sq.raw];
    usize index = pext(sliders::compressBlockers(blockers & t.mask), t.compressed_mask);
    return t.ptr[index];
  }

  inline constexpr auto rook(Square sq, Bitboard blockers) -> Bitboard {
    const sliders::SliderTable &t = sliders::rook_lut[sq.raw];
    usize index = pext(sliders::compressBlockers(blockers & t.mask), t.compressed_mask);
    return t.ptr[index];
  }

  inline constexpr auto lance(Square sq, Color piece_color, Bitboard blockers) -> Bitboard {
    const sliders::SliderTable &t = sliders::lance_lut[std::to_underlying(piece_color)][sq.raw];
    usize index = pext(sliders::compressBlockers(blockers & t.mask), t.compressed_mask);
    return t.ptr[index];
  }

  constexpr auto allBishops(Bitboard pieces, Bitboard blockers) -> Bitboard {
    Bitboard result{};
    for (Square sq : pieces)
      result |= bishop(sq, blockers);
    return result;
  }

  constexpr auto allRooks(Bitboard pieces, Bitboard blockers) -> Bitboard {
    Bitboard result{};
    for (Square sq : pieces)
      result |= rook(sq, blockers);
    return result;
  }

  constexpr auto allLances(Bitboard pieces, Color piece_color, Bitboard blockers) -> Bitboard {
    Bitboard result{};
    for (Square sq : pieces)
      result |= lance(sq, piece_color, blockers);
    return result;
  }

} // namespace lb::attacks
