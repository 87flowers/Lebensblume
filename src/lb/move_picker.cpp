#include "lb/move_picker.h"

#include <utility>

#include "lb/is_legal.h"
#include "lb/movegen.h"

namespace lb {

  MovePicker::MovePicker(const Game &game, Move tt_move) : game(game), tt_move(tt_move) {}

  auto MovePicker::next() -> Move {
    switch (stage) {
    case Stage::tt_move:
      stage = Stage::generate_moves;
      if (tt_move != Move::none() && isMoveLegal(game.position(), tt_move)) {
        return tt_move;
      }
      [[fallthrough]];
    case Stage::generate_moves:
      movegen::generateMoves(moves, game.position());
      stage = Stage::emit_moves;
      [[fallthrough]];
    case Stage::emit_moves:
      while (current_index < moves.size() && moves[current_index] == tt_move)
        current_index++;
      if (current_index >= moves.size())
        return Move::none();
      return moves[current_index++];
    }
    std::unreachable();
  }

} // namespace lb
