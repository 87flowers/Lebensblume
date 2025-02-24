#include "lb/attacks.h"

#include <array>
#include <ranges>

#include "lb/common.h"
#include "lb/types.h"
#include "lb/util/assert.h"
#include "lb/util/pext.h"

namespace lb::attacks::sliders {
  static constexpr auto generateAttacks(Bitboard piece, auto directions, Bitboard blockers) -> Bitboard {
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

  template <usize N, auto directions> static auto generateAttacks(std::array<u32, N> &result) -> void {
    const BlockerArray blocker_array = generatePotentialBlockers(directions);
    u64 offset = 0;
    for (u8 i : std::views::iota(0, 81)) {
      const Bitboard piece = Bitboard::fromSq(Square{i});
      const Bitboard blockers = blocker_array[i];
      const u64 compressed_blockers = compressBlockers(blockers);
      const u64 attack_mask = static_cast<u64>(generateAttacks(piece, directions, Bitboard{}).raw);
      const u64 count = 1 << blockers.count();
      u64 current_compressed = 0;
      for (u64 j : std::views::iota((u64)0, count)) {
        const Bitboard current = Bitboard{decompressBlockers(current_compressed, blockers)};
        const Bitboard attacks = generateAttacks(piece, directions, current);
        const u32 v = compressAttacks(attacks, attack_mask);
        result[offset + j] = v;
        lb_assert(decompressAttacks(v, attack_mask) == attacks);
        current_compressed = pinc<u64>(current_compressed, compressed_blockers);
      }
      lb_assert(current_compressed == 0);
      offset += count;
    }
    lb_assert(offset == N);
  }

  static consteval auto generateSliderTables(auto directions, const BlockerArray &blocker_array, const u32 *base_ptr) -> std::array<SliderTable, 81> {
    u32 offset = 0;
    std::array<SliderTable, 81> result;
    for (u8 i : std::views::iota(0, 81)) {
      const Bitboard piece = Bitboard::fromSq(Square{i});
      const Bitboard blockers = blocker_array[i];
      const u64 compressed_blockers = compressBlockers(blockers);
      result[i].blocker_mask = blockers;
      result[i].attack_mask = static_cast<u64>(generateAttacks(piece, directions, Bitboard{}).raw);
      result[i].ptr = &base_ptr[offset];
      offset += 1 << blockers.count();
    }
    return result;
  }

  constexpr BlockerArray bishop_blockers = generatePotentialBlockers(bishop_dirs);
  constexpr BlockerArray rook_blockers = generatePotentialBlockers(rook_dirs);
  constexpr BlockerArray lance_sente_blockers = generatePotentialBlockers(lance_sente_dirs);
  constexpr BlockerArray lance_gote_blockers = generatePotentialBlockers(lance_gote_dirs);

  constexpr usize bishop_count = generateAttackCount(bishop_blockers);
  constexpr usize rook_count = generateAttackCount(rook_blockers);
  constexpr usize lance_count = generateAttackCount(lance_sente_blockers);

  std::array<u32, bishop_count> bishop_attacks;
  std::array<u32, rook_count> rook_attacks;
  std::array<u32, lance_count> lance_sente_attacks;
  std::array<u32, lance_count> lance_gote_attacks;

  static auto _ = [] {
    generateAttacks<bishop_count, bishop_dirs>(bishop_attacks);
    generateAttacks<rook_count, rook_dirs>(rook_attacks);
    generateAttacks<lance_count, lance_sente_dirs>(lance_sente_attacks);
    generateAttacks<lance_count, lance_gote_dirs>(lance_gote_attacks);
    return nullptr;
  }();

  const std::array<SliderTable, 81> bishop_lut = generateSliderTables(bishop_dirs, bishop_blockers, bishop_attacks.data());
  const std::array<SliderTable, 81> rook_lut = generateSliderTables(rook_dirs, rook_blockers, rook_attacks.data());
  const std::array<std::array<SliderTable, 81>, 2> lance_lut = {
      generateSliderTables(lance_sente_dirs, lance_sente_blockers, lance_sente_attacks.data()),
      generateSliderTables(lance_gote_dirs, lance_gote_blockers, lance_gote_attacks.data()),
  };
} // namespace lb::attacks::sliders
