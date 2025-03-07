#include <string_view>
#include <tuple>
#include <vector>

#include "lb/common.h"
#include "lb/movegen.h"
#include "lb/position.h"
#include "lb/util/assert.h"

using namespace lb;

auto uchifuzume() -> void {
  std::vector<std::tuple<std::string_view, bool, std::string_view>> cases{{
      {"9/9/7gp/7pk/9/7G1/9/PPPPPPPP1/K8 b P 1", true, "P*1e"},
      {"9/9/7pp/7sk/9/7G1/9/PPPPPPPP1/K8 b P 1", false, "P*1e"},
      {"9/9/8p/6K1k/9/7G1/9/PPPPPPPP1/9 b P 1", true, "P*1e"},
      {"9/9/8p/6K1k/9/9/9/PPPPPPPP1/9 b P 1", false, "P*1e"},
      {"9/9/7gp/1R5gk/9/7G1/9/PPPPPPPP1/K8 b P 1", true, "P*1e"},
      {"9/9/7gp/7gk/9/7G1/9/PPPPPPPP1/K8 b P 1", false, "P*1e"},
      {"l6nl/5+P1gk/2np1S3/p1p4Pp/3P2Sp1/1PPb2PnP/P5GS1/R2g5/LN3Kb1L w RGs5p 5", false, "P*4h"},
      {"3lkl3/3p1p3/9/9/7B1/9/PPPP1PPPP/4r4/K3R4 b P 1", false, "P*5b"},
      {"3ngn3/3lkl3/3p1p3/9/9/7B1/PPPP1PPPP/9/4K4 b P 1", false, "P*5c"},
  }};
  for (auto [sfen, is_uchifuzume, move_str] : cases) {
    const Position position = Position::parse(sfen).value();
    const Move move = Move::parse(move_str).value();

    const bool direct_result = movegen::isUchifuzume(position, position.getKingSq(invert(position.activeColor())), Bitboard::fromSq(move.to()));
    lb_assert(direct_result == is_uchifuzume, "{} | {} | {} | {}", position, move, direct_result, is_uchifuzume);
  }
}

auto main() -> int {
  uchifuzume();
  return 0;
}
