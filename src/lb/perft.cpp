#include "lb/perft.h"

#include <print>

#include "lb/board.h"
#include "lb/common.h"
#include "lb/movegen.h"
#include "lb/types.h"

namespace lb::perft {

  template <bool print> static auto core(const Board &board, usize depth) -> usize {
    if (depth == 0)
      return 1;

    usize result = 0;

    movegen::MoveList moves;
    movegen::generateMoves(moves, board);

    if (depth == 1 && !print)
      return moves.size();

    for (Move m : moves) {
      const Board new_board = board.move(m);
      const usize child = core<false>(new_board, depth - 1);
      if constexpr (print) {
        std::print("{}: {}\n", m, child);
      }
      result += child;
    }

    return result;
  }

  auto value(const Board &board, usize depth) -> usize { return core<false>(board, depth); }

  auto run(const Board &board, usize depth) -> void {
    const auto start = time::Clock::now();
    const usize total = core<true>(board, depth);
    const auto end = time::Clock::now();

    const time::FloatSeconds elapsed = end - start;

    std::print("total: {}\n", total);
    std::print("perft to depth {} complete in {:.1f}ms ({:.1f} Mnps)\n", depth, elapsed.count() * 1000, total / (1'000'000 * elapsed.count()));
  }

} // namespace lb::perft
