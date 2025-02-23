#include <print>
#include <string_view>
#include <tuple>
#include <vector>

#include "lb/board.h"
#include "lb/perft.h"
#include "lb/types.h"
#include "lb/util/assert.h"

using namespace lb;

auto main() -> int {
  std::vector<std::tuple<std::string_view, std::vector<usize>>> cases{{
      {"l6nl/5+P1gk/2np1S3/p1p4Pp/3P2Sp1/1PPb2P1P/P5GS1/R8/LN4bKL w GR5pnsg 1", {1, 207, 28684, 4809015, 516925165}},
      {"lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b - 1", {1, 30, 900, 25470, 719731, 19861490, 547581517, 15086269607}},
      {"9/9/9/3k5/9/5K3/9/9/9 b RB2G2S2N2L9Prb2g2s2n2l9p 1", {1, 524, 248257, 112911856, 50852853772}},
      {"8l/1l+R2P3/p2pBG1pp/kps1p4/Nn1P2G2/P1P1P2PP/1PS6/1KSG3+r1/LN2+p3L w Sbgn3p 124", {1, 178, 18041, 2552846, 207741677, 24120401335}},
  }};

  for (auto [sfen, expected_results] : cases) {
    const Board board = Board::parse(sfen).value();
    std::print("position {}\n", board);
    for (usize i = 0; i < expected_results.size(); i++) {
      const usize result = perft::value(board, i);
      std::print("depth {} : {} : {}\n", i, result, expected_results[i]);
      lb_assert(result == expected_results[i]);
    }
  }

  return 0;
}
