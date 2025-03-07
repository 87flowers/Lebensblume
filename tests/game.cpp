#include <format>
#include <string_view>
#include <vector>

#include <print>

#include "lb/common.h"
#include "lb/game.h"
#include "lb/position.h"
#include "lb/util/assert.h"
#include "lb/util/tokenizer.h"

using namespace lb;

auto sennichite() -> void {
  const std::vector<std::tuple<std::string_view, std::string_view, std::string_view, std::string_view>> cases{{
      {
          "lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b - 1",
          "2h7h 8b9b 7h6h 9b8b",
          "6h7h 8b9b 7h6h 9b8b 6h7h 8b9b 7h6h 9b8b",
          "6h7h",
      },
      {
          "l7l/g1S6/k1n4pp/ppN3p2/2p4P1/P2P1bP2/KSN5P/1G7/L1bS4L w 2RSN5P2g2p 1",
          "G*8i R*7h 8i8h 7h8h G*8i G*7h 8i8h 7h8h G*8i G*7h",
          "8i8h 7h8h G*8i G*7h 8i8h 7h8h G*8i G*7h",
          "8i8h",
      },
  }};
  for (const auto [sfen, move_list1, move_list2, last_move] : cases) {
    Game game;
    game.setPosition(Position::parse(sfen).value());
    lb_dbg("{}\n", game.position());
    lb_assert(game.checkRepetition() == RepetitionType::none);
    lb_assert(game.checkMaybeRepetition() == RepetitionType::none);

    Tokenizer it1{move_list1};
    while (!it1.rest().empty()) {
      const Move m = Move::parse(it1.next()).value();
      lb_dbg("{}\n", m);
      game.move(m);
      lb_assert(game.checkRepetition() == RepetitionType::none);
      lb_assert(game.checkMaybeRepetition() == RepetitionType::none);
    }

    Tokenizer it2{move_list2};
    while (!it2.rest().empty()) {
      const Move m = Move::parse(it2.next()).value();
      lb_dbg("{}\n", m);
      game.move(m);
      lb_assert(game.checkRepetition() == RepetitionType::none);
      lb_assert(game.checkMaybeRepetition() == RepetitionType::sennichite);
    }

    lb_dbg("{}\n", last_move);
    game.move(Move::parse(last_move).value());
    lb_assert(game.checkRepetition() == RepetitionType::sennichite);
    lb_assert(game.checkMaybeRepetition() == RepetitionType::sennichite);
  }
}

auto repetition() -> void {
  const std::vector<std::tuple<std::string_view, std::string_view, std::string_view>> cases{{
      {
          "2k6/r8/9/9/9/9/9/9/2K6 b - 1",
          "7i6i 9b6b 6i7i 6b7b 7i6i "
          "7b6b 6i7i 6b7b 7i6i 7b6b 6i7i 6b7b 7i6i",
          "7b6b",
      },
      {
          "lnsgkgsnl/6rb1/ppppp1ppp/5p3/9/P8/1PPPPPPPP/LB5R1/1NSGKGSNL b - 4",
          "2h7h 3c3d 7h6h 6a7b 6h7h 5a6a 7h5h 8c8d 5h7h 5c5d 7h6h 8d8e 6h7h 3b4b 7h5h 8e8f 8h9g 1a1b 9g8f 6a5b 8f4b+ 5b6b 4b4a 1c1d 5h8h 3d3e 8h7h "
          "2b1c 4a2c 3a2b 2c1d 3e3f 1d3b 9a9b 3b2a 1c5g 2a2b 4d4e 2b4d 6b6a 4d3d 6a6b "
          "3d4d 6b5a 4d3c 5a6a 3c3d 6a5a 3d3c 5a4a 3c2c 4a5b 2c1b 7a8b 1b3d 5b4a 3d2c 4a5a 2c3c 5a4a 3c2c 4a5b 2c3d 5b5a 3d3c 5a6a 3c4c 6a6b 4c4d "
          "6b5a 4d3c 5a6a 3c4c 6a5a",
          "4c3c",
      },
      {
          "ln1gkgsnl/1rs4b1/pppppppp1/8p/9/4P4/PPPP1PPPP/1B2R4/LNSGKGSNL b - 4",
          "9g9f 3c3d 8h9g 1a1b 9g5c+ 2c2d 5c4c 2a1c 4c3d 9c9d 3d1b 3a3b 1b2b 6a6b 2b1c 8a9c 1c1d 6b5b 5h9h 5a4b 9h8h 3b2a L*4f 4b3a 1d4a 3a2b 4a5b "
          "8b8a 5b6b 2a1b 6b7b 9d9e 7b8a 1b2a 8a9a 2a3b 9f9e 6c6d 9a7c 2d2e 7c6d 3b2c 8h9h 2b2a 9h7h 8c8d 7h9h 2e2f 2g2f 2a2b 9h8h 2c2d 8h9h 9c8e "
          "6d5e 2b2a 5e6e 2a3a 6e7e 3a2a 7e6e 2a2b 6e5e 2b1b 5e4e 1b2a 4e5d 2a1b 5d4e 1b1a 4e4d 1a2a 4d5d 2a1a 5d4d 1a1b 4d4e 1b2a 4e5d 2a1a 5d4d "
          "1a2a",
          "4d5d",
      },
      {
          "lnsgk1snl/r5gb1/ppppppppp/9/9/9/PPPPPPPPP/1BR1G4/LNS1KGSNL b - 4",
          "9g9f 9c9d 7h6h 9b6b 6h6i 5c5d 6i6h 9a9b 6h6i 9b9c 6i6h 6c6d 6h6i 6a5b 6i6h 6b6c 7g7f 5b6b 8h9g 5a6a 6h6i 6b5b 6i6h 1c1d 6h6i 6a5a 7i7h "
          "7c7d 9g8h 7a7b 8h9g 1d1e 9g8h 7d7e 7f7e 5a4a 6i7i 2a1c 1g1f 5b6b 1f1e 6b6a 7i6i 1c2e 2g2f 4a4b 2f2e 6d6e 6i7i 1a1c 1e1d 6a5b 1d1c+ 5b5a "
          "1c2b 5d5e 2b3b 4b3b 8h5e P*5b 7i6i 3b4a 5e3c+ 8c8d 3c2c 3a3b 2c5f 4a4b 6i7i 8a7c 7i6i 7b6a 6i6h 3b2a 5f4f 8d8e 6h6i 4b4a 6i7i 6a6b 7i6i "
          "2a2b 6i7i 5a6a 4f5f 2b2c 5f2c 4a3a 7i6i 6e6f 2c1c 3a4a 1c1d 4a4b 1d1e 4b3b 6g6f 6c5c 1e1d 3b2b 1d1c 2b3c 1c2d 3c3b 2d1d 3b3c 1d2d 3c3b "
          "2d1d 3b3a 1d1c 3a4a 1c2c 4a3a 2c1c 3a3b",
          "1c1d",
      },
  }};
  for (const auto [sfen, move_list, last_move] : cases) {
    Game game;
    game.setPosition(Position::parse(sfen).value());
    lb_dbg("{}\n", game.position());
    lb_assert(game.checkRepetition() == RepetitionType::none);

    Tokenizer it{move_list};
    while (!it.rest().empty()) {
      const Move m = Move::parse(it.next()).value();
      lb_dbg("{}\n", m);
      game.move(m);
      lb_assert(game.checkRepetition() == RepetitionType::none);
    }

    lb_dbg("{}\n", last_move);
    game.move(Move::parse(last_move).value());
    lb_assert(game.checkRepetition() == RepetitionType::illegal_perpetual);
    lb_assert(game.checkMaybeRepetition() == RepetitionType::illegal_perpetual);
  }
}

auto main() -> int {
  sennichite();
  repetition();
  return 0;
}
