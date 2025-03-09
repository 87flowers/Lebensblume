#include "lb/eval/hce.h"
#include "lb/eval/eval.h"

#include <array>
#include <utility>

#include "lb/common.h"
#include "lb/geometry.h"
#include "lb/position.h"
#include "lb/types.h"

namespace lb::eval {

  static auto calcInfluenceScores(const Position &position) -> std::tuple<i32, i32> {
    constexpr std::array<i32, 9> multipler{{0, 1024, 1024 / 4, 1024 / 9, 1024 / 16, 1024 / 25, 1024 / 36, 1024 / 49, 1024 / 81}};
    constexpr auto count_influence = [](Bitboard influence, Bitboard ring) -> i32 { return static_cast<i32>((influence & ring).count()); };

    const auto sente = position.activeColor() == Color::gote ? position.getDanger() : position.getDefended();
    const auto gote = position.activeColor() == Color::sente ? position.getDanger() : position.getDefended();

    const Square sente_king_sq = position.getKingSq(Color::sente);
    const Square gote_king_sq = position.getKingSq(Color::gote);

    i32 sente_score = 0;
    i32 gote_score = 0;

    for (usize dist = 1; dist < 9; dist++) {
      const Bitboard sente_ring = geometry::manhattanRing(sente_king_sq, dist);
      const Bitboard gote_ring = geometry::manhattanRing(gote_king_sq, dist);

      sente_score += (40 * count_influence(sente, sente_ring) - 70 * count_influence(gote, sente_ring)) / static_cast<i32>(dist * dist);
      gote_score += (40 * count_influence(gote, gote_ring) - 70 * count_influence(sente, gote_ring)) / static_cast<i32>(dist * dist);
    }

    return {sente_score, gote_score};
  }

  auto hce(const Position &position) -> i32 {

    const auto board = [&](Color color) -> i32 {
      i32 result = 0;
      result += 100 * position.getPiece(color, PieceType::pawn).count();
      result += 300 * position.getPiece(color, PieceType::lance).count();
      result += 400 * position.getPiece(color, PieceType::knight).count();
      result += 500 * position.getPiece(color, PieceType::silver).count();
      result += 600 * position.getPiece(color, PieceType::gold).count();
      result += 600 * position.getPromoteds(color).count();
      result += 650 * position.getPiece(color, PieceType::tokin).count();
      result += 800 * position.getPiece(color, PieceType::bishop).count();
      result += 1000 * position.getPiece(color, PieceType::horse).count();
      result += 1000 * position.getPiece(color, PieceType::rook).count();
      result += 1200 * position.getPiece(color, PieceType::dragon).count();
      return result;
    };

    const auto hand = [&](Color color) -> i32 {
      const Hand hand = position.getHand(color);
      i32 result = 0;
      result += 110 * hand.getPiece(PieceType::pawn);
      result += 330 * hand.getPiece(PieceType::lance);
      result += 440 * hand.getPiece(PieceType::knight);
      result += 550 * hand.getPiece(PieceType::silver);
      result += 660 * hand.getPiece(PieceType::gold);
      result += 880 * hand.getPiece(PieceType::bishop);
      result += 1100 * hand.getPiece(PieceType::rook);
      return result;
    };

    const auto [sente_influence, gote_influence] = calcInfluenceScores(position);

    const i32 sente_eval = board(Color::sente) + hand(Color::sente) + sente_influence;
    const i32 gote_eval = board(Color::gote) + hand(Color::gote) + gote_influence;

    const i32 fudge = static_cast<i32>(position.getHash() & 0xF) - 0x7;

    switch (position.activeColor()) {
    case Color::sente:
      return clamp(sente_eval - gote_eval + fudge);
    case Color::gote:
      return clamp(gote_eval - sente_eval - fudge);
    default:
      std::unreachable();
    }
  }

} // namespace lb::eval
