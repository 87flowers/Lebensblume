#pragma once

#include "lb/common.h"
#include "lb/types.h"
#include "lb/util/static_vector.h"

namespace lb {
  struct Position;
} // namespace lb

namespace lb::movegen {

  using MoveList = StaticVector<Move, max_legal_moves>;

  auto validNormalDests(Color active_color, PieceType ptype) -> Bitboard;
  auto isUchifuzume(const Position &position, Square enemy_king, Bitboard drop_bb) -> bool;

  auto generateMoves(MoveList &moves, const Position &position) -> void;
  auto generateNoises(MoveList &moves, const Position &position) -> void;
  auto generateQuiets(MoveList &moves, const Position &position) -> void;

} // namespace lb::movegen
