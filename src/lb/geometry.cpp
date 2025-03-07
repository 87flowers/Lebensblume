#include "lb/geometry.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <ranges>

#include "lb/common.h"

namespace lb::geometry {

  using RayTable = std::array<std::array<Bitboard, 81>, 81>;

  static constexpr RayTable ray_between_table = []() constexpr {
    RayTable result{};
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

  static constexpr RayTable ray_infinite_table = []() constexpr {
    RayTable result{};
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

  using RingTable = std::array<std::array<Bitboard, 9>, 81>;

  static constexpr RingTable manhattan_ring = []() constexpr {
    constexpr auto abs_sub = [](usize a, usize b) constexpr { return (a > b) ? a - b : b - a; };

    RingTable result{};
    for (usize d : std::views::iota(0, 9)) {
      for (u8 i : std::views::iota(0, 81)) {
        const Square center{i};
        const auto [cfile, crank] = center.toFileAndRank();
        for (u8 j : std::views::iota(0, 81)) {
          const Square test{j};
          const auto [tfile, trank] = test.toFileAndRank();
          const usize distance = std::max(abs_sub(cfile, tfile), abs_sub(crank, trank));
          if (distance == d)
            result[center.raw][d].set(test);
        }
      }
    }
    return result;
  }();

  auto manhattanRing(Square center, usize distance) -> Bitboard {
    if (distance >= manhattan_ring[0].size())
      return {};
    return manhattan_ring[center.raw][distance];
  }

} // namespace lb::geometry
