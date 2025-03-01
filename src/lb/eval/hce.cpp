#include "lb/eval/hce.h"
#include "lb/eval/eval.h"

#include <utility>

#include "lb/board.h"
#include "lb/common.h"
#include "lb/types.h"

namespace lb::eval {

  auto hce(const Board &position) -> i32 {

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
      result += 100 * hand.getPiece(PieceType::pawn);
      result += 300 * hand.getPiece(PieceType::lance);
      result += 400 * hand.getPiece(PieceType::knight);
      result += 500 * hand.getPiece(PieceType::silver);
      result += 600 * hand.getPiece(PieceType::gold);
      result += 800 * hand.getPiece(PieceType::bishop);
      result += 1000 * hand.getPiece(PieceType::rook);
      return result;
    };

    const i32 sente_eval = board(Color::sente) + hand(Color::sente);
    const i32 gote_eval = board(Color::gote) + hand(Color::gote);

    switch (position.activeColor()) {
    case Color::sente:
      return sente_eval - gote_eval;
    case Color::gote:
      return gote_eval - sente_eval;
    default:
      std::unreachable();
    }
  }

} // namespace lb::eval
