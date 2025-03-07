#include <print>
#include <ranges>
#include <string_view>
#include <tuple>
#include <vector>

#include "lb/is_legal.h"
#include "lb/perft.h"
#include "lb/position.h"
#include "lb/types.h"
#include "lb/util/assert.h"

using namespace lb;

template <bool print> auto isLegalPerft(const Position &position, usize depth) -> usize {
  if (depth == 0)
    return 1;

  const Color color = position.activeColor();

  usize result = 0;
  for (const u8 f : std::views::iota(0, 81)) {
    for (const u8 t : std::views::iota(0, 81)) {
      const Square from{f};
      const Square to{t};
      if (from == to)
        continue;

      const Place src = position.getPlace(from);
      if (src.empty())
        continue;

      const bool can_promo = from.isPromoSquare(color) || to.isPromoSquare(color);
      for (const bool promo : {false, true}) {
        if (promo && !can_promo)
          continue;

        const Move m = Move::makeMove(from, to, promo);
        if (isMoveLegal(position, m)) {
          const Position new_position = position.move(m);
          const usize child = isLegalPerft<false>(new_position, depth - 1);
          if constexpr (print) {
            std::print("{}: {}\n", m, child);
          }
          result += child;
        }
      }
    }
  }

  using PT = PieceType;
  for (const PieceType ptype : {PT::pawn, PT::bishop, PT::rook, PT::lance, PT::knight, PT::silver, PT::gold}) {
    for (const u8 t : std::views::iota(0, 81)) {
      const Square to{t};
      const Move m = Move::makeDrop(ptype, to);
      if (isMoveLegal(position, m)) {
        const Position new_position = position.move(m);
        const usize child = isLegalPerft<false>(new_position, depth - 1);
        if constexpr (print) {
          std::print("{}: {}\n", m, child);
        }
        result += child;
      }
    }
  }

  return result;
}

auto main() -> int {
  std::vector<std::tuple<std::string_view, std::vector<usize>>> cases{{
      {"8l/1l+R2P3/p2pBG1pp/kps1p4/Nn1P2G2/P1P1P2PP/1PS6/SKS+r5/LN2+p3L w b2gn3p 126", {1, 179}},
      {"8l/1l+R2P3/p2pBG1pp/kps1p4/Nn1P2G2/P1P1P2PP/1PS6/1KS+r5/LN2+p3L b Sb2gn3p 125", {1, 101, 17765}},
      {"l6nl/5+P1gk/2np1S3/p1p4P1/3P2Spp/1PPb2P1P/P5GS1/R8/LN4bKL b RGgsn5p 2", {1, 146}},
      {"l6nl/5+P1gk/2np1S3/p1p4Pp/3P2Sp1/1PPb2P1P/P5GS1/R8/LN4bKL w GR5pnsg 1", {1, 207, 28684, 4809015}},
      {"lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b - 1", {1, 30, 900, 25470, 719731, 19861490}},
      {"9/9/9/3k5/9/5K3/9/9/9 b RB2G2S2N2L9Prb2g2s2n2l9p 1", {1, 524, 248257}},
      {"8l/1l+R2P3/p2pBG1pp/kps1p4/Nn1P2G2/P1P1P2PP/1PS6/1KSG3+r1/LN2+p3L w Sbgn3p 124", {1, 178, 18041, 2552846}},
      {"l6nl/5+P1gk/2np1S3/p1p4Pp/3P2Sp1/1PPb2P1P/P5GS1/R2g2K2/LN4b1L w RGsn5p 3", {1, 160, 20791, 2685299}},
      {"lnsgkgsnl/1r5b1/9/9/9/9/9/1B5R1/LNSGKGSNL b 9P9p 1", {1, 119, 13311, 1473109}},
      {"kl7/1p7/9/9/9/9/9/7P1/7LK b RB2G2S2N2L8Prb2g2s2nl8p 1", {1, 492, 229547}},
      {"ppplkl1pp/b2p1p3/9/9/9/B8/9/5P1P1/PPPP1LKLP b R2G2S2N2Pr2g2s2n2p 1", {1, 260, 62558, 14650255}},
  }};

  for (auto [sfen, expected_results] : cases) {
    const Position position = Position::parse(sfen).value();
    std::print("position {}\n", position);
    for (usize i = 0; i < expected_results.size(); i++) {
      const usize result = isLegalPerft<true>(position, i);
      std::print("depth {} : {} : {}\n", i, result, expected_results[i]);
      lb_assert(result == expected_results[i]);
    }
  }

  return 0;
}
