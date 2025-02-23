#include "lb/attacks.h"

#include <array>
#include <ranges>

#include "lb/assert.h"
#include "lb/bit.h"
#include "lb/common.h"
#include "lb/types.h"

namespace lb::attacks::sliders {
  using BlockerArray = std::array<Bitboard, 81>;

  static consteval auto generatePotentialBlockers(Bitboard piece, auto directions) -> Bitboard {
    Bitboard result{};
    for (Direction dir : directions)
      for (Bitboard current = piece.shift(dir); !current.shift(dir).empty(); current = current.shift(dir))
        result |= current;
    return result;
  }

  static consteval auto generatePotentialBlockers(auto directions) -> BlockerArray {
    BlockerArray result;
    for (u8 i : std::views::iota(0, 81)) {
      const Bitboard piece = Bitboard::fromSq(Square{i});
      result[i] = generatePotentialBlockers(piece, directions);
    }
    return result;
  }

  static consteval auto generateAttackCount(const BlockerArray &blocker_array) -> usize {
    usize result = 0;
    for (Bitboard b : blocker_array) {
      const int count = std::popcount(compressBlockers(b));
      result += 1 << count;
    }
    return result;
  }

  static consteval auto generateAttacks(Bitboard piece, auto directions, Bitboard blockers) -> Bitboard {
    Bitboard result{};
    for (Direction dir : directions) {
      Bitboard current = piece.shift(dir);
      while (!current.empty()) {
        result |= current;
        if (!(current & blockers).empty())
          break;
        current = current.shift(dir);
      }
    }
    return result;
  }

  template <usize N> static consteval auto generateAttacks(auto directions, const BlockerArray &blocker_array) -> std::array<Bitboard, N> {
    std::array<Bitboard, N> result;
    u64 offset = 0;
    for (u8 i : std::views::iota(0, 81)) {
      const Bitboard piece = Bitboard::fromSq(Square{i});
      const Bitboard blockers = blocker_array[i];
      const u64 compressed_blockers = compressBlockers(blockers);
      const u64 count = 1 << std::popcount(compressed_blockers);
      u64 current_compressed = 0;
      for (u64 j : std::views::iota((u64)0, count)) {
        const Bitboard current = Bitboard{decompressBlockers(current_compressed, blockers)};
        result[offset + j] = generateAttacks(piece, directions, current);
        current_compressed = pinc<u64>(current_compressed, compressed_blockers);
      }
      lb_assert(current_compressed == 0);
      offset += count;
    }
    lb_assert(offset == N);
    return result;
  }

  static consteval auto generateSliderTables(auto directions, const BlockerArray &blocker_array, const Bitboard *base_ptr)
      -> std::array<SliderTable, 81> {
    u32 offset = 0;
    std::array<SliderTable, 81> result;
    for (u8 i : std::views::iota(0, 81)) {
      const Bitboard piece = Bitboard::fromSq(Square{i});
      const Bitboard blockers = blocker_array[i];
      const u64 compressed_blockers = compressBlockers(blockers);
      result[i].mask = blockers;
      result[i].compressed_mask = compressed_blockers;
      result[i].ptr = &base_ptr[offset];
      offset += 1 << std::popcount(compressed_blockers);
    }
    return result;
  }

  constexpr std::array<Direction, 4> bishop_dirs = {Direction::ne, Direction::se, Direction::sw, Direction::nw};
  constexpr std::array<Direction, 4> rook_dirs = {Direction::n, Direction::e, Direction::s, Direction::w};
  constexpr std::array<Direction, 1> lance_sente_dirs = {Direction::n};
  constexpr std::array<Direction, 1> lance_gote_dirs = {Direction::s};

  constexpr BlockerArray bishop_blockers = generatePotentialBlockers(bishop_dirs);
  constexpr BlockerArray rook_blockers = generatePotentialBlockers(rook_dirs);
  constexpr BlockerArray lance_sente_blockers = generatePotentialBlockers(lance_sente_dirs);
  constexpr BlockerArray lance_gote_blockers = generatePotentialBlockers(lance_gote_dirs);

  constexpr usize bishop_count = generateAttackCount(bishop_blockers);
  constexpr usize rook_count = generateAttackCount(rook_blockers);
  constexpr usize lance_count = generateAttackCount(lance_sente_blockers);

  const std::array<Bitboard, bishop_count> bishop_attacks = generateAttacks<bishop_count>(bishop_dirs, bishop_blockers);
  const std::array<Bitboard, rook_count> rook_attacks = generateAttacks<rook_count>(rook_dirs, rook_blockers);
  const std::array<std::array<Bitboard, lance_count>, 2> lance_attacks = {
      generateAttacks<lance_count>(lance_sente_dirs, lance_sente_blockers),
      generateAttacks<lance_count>(lance_gote_dirs, lance_gote_blockers),
  };

  const std::array<SliderTable, 81> bishop_lut = generateSliderTables(bishop_dirs, bishop_blockers, bishop_attacks.data());
  const std::array<SliderTable, 81> rook_lut = generateSliderTables(rook_dirs, rook_blockers, rook_attacks.data());
  const std::array<std::array<SliderTable, 81>, 2> lance_lut = {
      generateSliderTables(lance_sente_dirs, lance_sente_blockers, lance_attacks[0].data()),
      generateSliderTables(lance_gote_dirs, lance_gote_blockers, lance_attacks[1].data()),
  };
} // namespace lb::attacks::sliders
