#include "lb/geometry.h"

#include <array>
#include <ranges>

#include "lb/common.h"

namespace lb::geometry {

  using Table = std::array<std::array<Bitboard, 81>, 81>;

  static const constinit Table ray_between_table = []() constexpr {
    Table result{};
    for (u8 i : std::views::iota(0, 81)) {
      const Bitboard from = Bitboard::fromSq(Square{i});
      for (Direction dir : {Direction::n, Direction::ne, Direction::e, Direction::se, Direction::s, Direction::sw, Direction::w, Direction::nw}) {
        Bitboard bb{};
        for (Bitboard to = from.shift(dir); !to.empty(); to = to.shift(dir)) {
          result[from.toSq().raw][to.toSq().raw] = bb;
          bb |= to;
        }
      }
    }
    return result;
  }();

  static const constinit Table ray_infinite_table = []() constexpr {
    Table result{};
    for (u8 i : std::views::iota(0, 81)) {
      const Bitboard from = Bitboard::fromSq(Square{i});
      for (Direction dir : {Direction::n, Direction::ne, Direction::e, Direction::se, Direction::s, Direction::sw, Direction::w, Direction::nw}) {
        Bitboard bb{};
        for (Bitboard to = from.shift(dir); !to.empty(); to = to.shift(dir)) {
          bb |= to;
        }
        for (Bitboard to = from.shift(dir); !to.empty(); to = to.shift(dir)) {
          result[from.toSq().raw][to.toSq().raw] = bb;
        }
      }
    }
    return result;
  }();

  auto rayBetween(Square a, Square b) -> Bitboard { return ray_between_table[a.raw][b.raw]; }
  auto rayInfinite(Square a, Square b) -> Bitboard { return ray_infinite_table[a.raw][b.raw]; }

} // namespace lb::geometry
