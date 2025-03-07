#include "lb/perft.h"

#include <print>

#include "lb/common.h"
#include "lb/movegen.h"
#include "lb/position.h"
#include "lb/types.h"

namespace lb::perft {

  template <bool print> static auto core(const Position &position, usize depth) -> usize {
    if (depth == 0)
      return 1;

    usize result = 0;

    movegen::MoveList moves;
    movegen::generateMoves(moves, position);

    if (depth == 1 && !print)
      return moves.size();

    for (Move m : moves) {
      const Position new_position = position.move(m);
      const usize child = core<false>(new_position, depth - 1);
      if constexpr (print) {
        std::print("{}: {}\n", m, child);
      }
      result += child;
    }

    return result;
  }

  auto value(const Position &position, usize depth) -> usize { return core<false>(position, depth); }

  auto run(const Position &position, usize depth) -> void {
    const auto start = time::Clock::now();
    const usize total = core<true>(position, depth);
    const auto end = time::Clock::now();

    const time::FloatSeconds elapsed = end - start;

    std::print("total: {}\n", total);
    std::print("perft to depth {} complete in {:.1f}ms ({:.1f} Mnps)\n", depth, elapsed.count() * 1000, total / (1'000'000 * elapsed.count()));
  }

} // namespace lb::perft
