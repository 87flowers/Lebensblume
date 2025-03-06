#pragma once

#include "lb/common.h"
#include "lb/game.h"
#include "lb/movegen.h"
#include "lb/types.h"

namespace lb {

  struct MovePicker {
  private:
    enum class Stage {
      tt_move,
      generate_moves,
      emit_moves,
    };

    Stage stage = Stage::tt_move;

    const Game &game;
    Move tt_move;

    usize current_index = 0;
    movegen::MoveList moves;

  public:
    MovePicker(const Game &game, Move tt_move);

    auto next() -> Move;
  };

} // namespace lb
