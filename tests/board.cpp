#include <format>
#include <string_view>
#include <vector>

#include <print>

#include "lb/assert.h"
#include "lb/board.h"

using namespace lb;

auto roundtrip() -> void {
  const std::vector<std::string_view> cases{{
      "lnsgkgsn1/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL w - 1",
      "lnsgk2nl/1r4gs1/p1pppp1pp/1p4p2/7P1/2P6/PP1PPPP1P/1SG4R1/LN2KGSNL b Bb 1",
      "ln1g5/1r2S1k2/p2pppn2/2ps2p2/1p7/2P6/PPSPPPPLP/2G2K1pr/LN4G1b w BGSLPnp 62",
      "8l/1l+R2P3/p2pBG1pp/kps1p4/Nn1P2G2/P1P1P2PP/1PS6/1KSG3+r1/LN2+p3L w Sbgn3p 124",
      "ln1g3nl/2s2k1+P1/p3pg3/1np2p2p/3p5/1SP3P1P/P1KPPSp2/2G6/L2b1G2L w RBSN2Pr2p 66",
  }};
  for (std::string_view sfen : cases) {
    const Board board = Board::parse(sfen).value();
    const std::string result = std::format("{}", board);
    lb_assert(result == sfen, "{} != {}", sfen, result);
  }
}

auto main() -> int {
  roundtrip();
  return 0;
}
