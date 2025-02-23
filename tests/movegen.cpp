#include <string_view>
#include <tuple>
#include <vector>

#include "lb/assert.h"
#include "lb/board.h"
#include "lb/common.h"
#include "lb/movegen.h"

using namespace lb;

auto uchifuzume() -> void {
  std::vector<std::tuple<std::string_view, bool, std::string_view>> cases{{
      {"9/9/7gp/7pk/9/7G1/9/PPPPPPPP1/K8 b P 1", true, "P*1e"},
      {"9/9/7pp/7sk/9/7G1/9/PPPPPPPP1/K8 b P 1", false, "P*1e"},
      {"9/9/8p/6K1k/9/7G1/9/PPPPPPPP1/9 b P 1", true, "P*1e"},
      {"9/9/8p/6K1k/9/9/9/PPPPPPPP1/9 b P 1", false, "P*1e"},
      {"9/9/7gp/1R5gk/9/7G1/9/PPPPPPPP1/K8 b P 1", true, "P*1e"},
      {"9/9/7gp/7gk/9/7G1/9/PPPPPPPP1/K8 b P 1", false, "P*1e"},
  }};
  for (auto [sfen, is_uchifuzume, move_str] : cases) {
    const Board board = Board::parse(sfen).value();
    const Move move = Move::parse(move_str).value();

    const bool direct_result = movegen::isUchifuzume(board, board.getKingSq(invert(board.activeColor())), Bitboard::fromSq(move.to()));
    lb_assert(direct_result == is_uchifuzume, "{} | {} | {} | {}", board, move, direct_result, is_uchifuzume);
  }
}

auto main() -> int {
  uchifuzume();
  return 0;
}
