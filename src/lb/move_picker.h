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
      generate_noises,
      emit_noises,
      generate_quiets,
      emit_quiets,
    };

    Stage stage = Stage::tt_move;

    const Game &game;
    Move tt_move;

    usize current_index = 0;
    movegen::MoveList moves;

    bool suppress_quiets = false;

  public:
    MovePicker(const Game &game, Move tt_move);

    auto next() -> Move;

    auto suppressQuiets() -> void { suppress_quiets = true; }
  };

} // namespace lb
