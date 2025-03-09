#include "lb/move_picker.h"

#include <utility>

#include "lb/is_legal.h"
#include "lb/movegen.h"

namespace lb {

  MovePicker::MovePicker(const Game &game, Move tt_move) : game(game), tt_move(tt_move) {}

  auto MovePicker::next() -> Move {
    switch (stage) {
    case Stage::tt_move:
      stage = Stage::generate_noises;
      if (tt_move != Move::none() && isMoveLegal(game.position(), tt_move)) {
        return tt_move;
      }
      [[fallthrough]];
    case Stage::generate_noises:
      moves.clear();
      movegen::generateNoises(moves, game.position());
      stage = Stage::emit_noises;
      [[fallthrough]];
    case Stage::emit_noises:
      while (current_index < moves.size() && moves[current_index] == tt_move)
        current_index++;
      if (current_index < moves.size())
        return moves[current_index++];
      stage = Stage::generate_quiets;
      [[fallthrough]];
    case Stage::generate_quiets:
      if (suppress_quiets)
        return Move::none();
      moves.clear();
      movegen::generateQuiets(moves, game.position());
      stage = Stage::emit_quiets;
      [[fallthrough]];
    case Stage::emit_quiets:
      if (suppress_quiets)
        return Move::none();
      while (current_index < moves.size() && moves[current_index] == tt_move)
        current_index++;
      if (current_index < moves.size())
        return moves[current_index++];
      return Move::none();
    }
    std::unreachable();
  }

} // namespace lb
