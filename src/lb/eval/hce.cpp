#include "lb/eval/hce.h"
#include "lb/eval/eval.h"

#include <array>
#include <print>
#include <tuple>
#include <utility>

#include "lb/common.h"
#include "lb/eval/influence.h"
#include "lb/geometry.h"
#include "lb/position.h"
#include "lb/types.h"

namespace lb::eval {

  static auto calcInfluenceScores(const Position &position) -> std::tuple<i32, i32> {
    const auto count_influence = [](const std::array<Bitboard, 4> &influence, Bitboard ring) -> i32 {
      return static_cast<i32>(1 * (influence[0] & ring).count() + 2 * (influence[1] & ring).count() + 4 * (influence[2] & ring).count() +
                              8 * (influence[3] & ring).count());
    };

    const auto sente = calcInfluence(Color::sente, position);
    const auto gote = calcInfluence(Color::gote, position);
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

  static auto boardScore(const Position &position, Color color) -> i32 {
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
  }

  static auto handScore(const Position &position, Color color) -> i32 {
    const Hand hand = position.getHand(color);
    i32 result = 0;
    result += 100 * hand.getPiece(PieceType::pawn);
    result += 300 * hand.getPiece(PieceType::lance);
    result += 400 * hand.getPiece(PieceType::knight);
    result += 500 * hand.getPiece(PieceType::silver);
    result += 600 * hand.getPiece(PieceType::gold);
    result += 800 * hand.getPiece(PieceType::bishop);
    result += 1000 * hand.getPiece(PieceType::rook);
    return result;
  }

  auto hce(const Position &position) -> i32 {

    const auto [sente_influence, gote_influence] = calcInfluenceScores(position);

    const i32 sente_eval = boardScore(position, Color::sente) + handScore(position, Color::sente) + sente_influence;
    const i32 gote_eval = boardScore(position, Color::gote) + handScore(position, Color::gote) + gote_influence;

    switch (position.activeColor()) {
    case Color::sente:
      return clamp(sente_eval - gote_eval);
    case Color::gote:
      return clamp(gote_eval - sente_eval);
    default:
      std::unreachable();
    }
  }

  auto printInfo(const Position &position) -> void {
    const auto [sente_influence, gote_influence] = calcInfluenceScores(position);
    std::print("sente board:     {}\n", boardScore(position, Color::sente));
    std::print("sente hand:      {}\n", handScore(position, Color::sente));
    std::print("sente influence: {}\n", sente_influence);
    std::print("gote board:      {}\n", boardScore(position, Color::gote));
    std::print("gote hand:       {}\n", handScore(position, Color::gote));
    std::print("gote influence:  {}\n", gote_influence);
    std::print("eval cp {}\n", hce(position));
  }

} // namespace lb::eval
