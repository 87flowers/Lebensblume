#include "lb/search.h"

#include <print>
#include <random>

#include "lb/game.h"
#include "lb/movegen.h"
#include "lb/types.h"
#include "lb/util/assert.h"

namespace lb::search {

  static std::mt19937_64 prng_engine{};

  static auto search(Game &game) -> void {
    movegen::MoveList moves;
    movegen::generateMoves(moves, game.position());

    if (moves.size() == 0) {
      std::print("bestmove null\n");
    } else {
      std::uniform_int_distribution<usize> rand{0, moves.size() - 1};
      const Move m = moves[rand(prng_engine)];
      std::print("info depth 0 score 0 nodes {} pv {}\n", moves.size(), m);
      std::print("bestmove {}\n", m);
    }
  }

  auto usiTime(Game &game, TimeSettings ts) -> void { search(game); }
  auto usiDepth(Game &game, i64 depth) -> void { search(game); }
  auto usiNode(Game &game, i64 nodes) -> void { search(game); }

} // namespace lb::search
