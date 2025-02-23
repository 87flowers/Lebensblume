#pragma once

#include "lb/common.h"
#include "lb/types.h"
#include "lb/util/static_vector.h"

namespace lb {
  struct Board;
} // namespace lb

namespace lb::movegen {

  using MoveList = StaticVector<Move, max_legal_moves>;

  auto isUchifuzume(const Board &board, Square enemy_king, Bitboard drop_bb) -> bool;

  auto generateMoves(MoveList &moves, const Board &board) -> void;

} // namespace lb::movegen
