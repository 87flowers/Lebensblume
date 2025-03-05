#pragma once

#include <vector>

#include "lb/board.h"
#include "lb/common.h"
#include "lb/tt.h"
#include "lb/types.h"
#include "lb/zhash.h"

namespace lb {

  enum class RepetitionType {
    none,
    // 千日手
    sennichite,
    // 連続王手の千日手
    // Most recent move played is illegal
    illegal_perpetual,
  };

  struct Game {
  private:
    std::vector<Board> position_stack;
    std::vector<zhash::Hash> hash_stack;
    std::vector<Move> move_stack;

    tt::TT transposition_table{default_hash_size};

  public:
    auto reset() -> void;

    auto position() const -> const Board & { return position_stack.back(); }
    auto hash() const -> zhash::Hash { return position().getHash(); }
    // Strict check
    auto checkRepetition() const -> RepetitionType;
    // Potentially repeating position
    auto checkMaybeRepetition() const -> RepetitionType;

    auto setPositionStartpos() -> void { setPosition(Board::startpos); }
    auto setPosition(const Board &new_pos) -> void {
      position_stack.clear();
      hash_stack.clear();
      move_stack.clear();
      position_stack.push_back(new_pos);
      hash_stack.push_back(new_pos.getHash());
    }

    auto move(Move m) -> void {
      position_stack.emplace_back(position_stack.back().move(m));
      hash_stack.push_back(position_stack.back().getHash());
      move_stack.push_back(m);
    }

    auto unmove() -> void {
      position_stack.pop_back();
      hash_stack.pop_back();
      move_stack.pop_back();
    }

    auto ttLoad(int ply) const -> tt::LookupResult { return transposition_table.load(hash(), ply); }
    auto ttStore(int ply, tt::LookupResult lr) -> void { transposition_table.store(hash(), ply, lr); }

    auto printKifu() const -> void;
  };

} // namespace lb
