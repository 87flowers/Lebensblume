#include "lb/is_legal.h"

#include "lb/attacks.h"
#include "lb/board.h"
#include "lb/common.h"
#include "lb/geometry.h"
#include "lb/movegen.h"

namespace lb {

  auto isMoveLegal(const Board &position, Move m) -> bool {
    const Color color = position.activeColor();

    if (m == Move::none())
      return false;

    if (m == Move::win())
      return position.canDeclareEnteringKingsWin();

    Bitboard valid_dests;
    switch (position.getCheckers().count()) {
    case 0:
      valid_dests = ~Bitboard{};
      break;
    case 1:
      valid_dests = geometry::rayBetween(position.getKingSq(color), position.getCheckers().toSq()) | position.getCheckers();
      break;
    case 2:
    default:
      valid_dests = {};
      break;
    }

    if (m.drop()) {
      const Square to = m.to();
      const PieceType ptype = m.ptype();
      const Bitboard to_bb = Bitboard::fromSq(to);

      lb_assert(ptype != PieceType::none && ptype != PieceType::king && !ptype.promoted());

      if (!position.getPlace(to).empty())
        return false;
      if (position.getHand(color).getPiece(ptype) == 0)
        return false;

      // 行き所のない駒
      valid_dests &= movegen::validNormalDests(color, ptype);

      if (ptype == PieceType::pawn) {
        // 打ち歩詰め
        const Bitboard enemy_king = position.getKing(invert(color));
        const Bitboard potential_uchifuzume = enemy_king.shiftRelative(Direction::n, invert(color));
        if (potential_uchifuzume == to_bb && movegen::isUchifuzume(position, enemy_king.toSq(), to_bb))
          return false;

        // 二歩
        valid_dests &= ~position.getPiece(color, PieceType::pawn).fillFiles();
      }

      if ((valid_dests & to_bb).empty())
        return false;

      return true;
    } else {
      const Square from = m.from();
      const Square to = m.to();
      const bool promo = m.promo();

      lb_assert(from != to);

      const Bitboard from_bb = Bitboard::fromSq(from);
      const Bitboard to_bb = Bitboard::fromSq(to);

      const Place src = position.getPlace(from);
      const Place dest = position.getPlace(to);
      const PieceType ptype = src.ptype();

      if (src.empty() || src.color() != color || ptype == PieceType::none)
        return false;
      if (promo && !ptype.promotable())
        return false;
      if (promo && !from.isPromoSquare(color) && !to.isPromoSquare(color))
        return false;
      if (!dest.empty() && dest.color() == color)
        return false;

      // 行き所のない駒
      if (!promo && (movegen::validNormalDests(color, ptype) & to_bb).empty())
        return false;

      switch (ptype.raw) {
      case PieceType::none:
        break;
      case PieceType::pawn:
        valid_dests &= attacks::pawn(from, color);
        break;
      case PieceType::bishop:
        valid_dests &= attacks::bishop(from, position.getOccupied());
        break;
      case PieceType::rook:
        valid_dests &= attacks::rook(from, position.getOccupied());
        break;
      case PieceType::lance:
        valid_dests &= attacks::lance(from, color, position.getOccupied());
        break;
      case PieceType::knight:
        valid_dests &= attacks::knight(from, color);
        break;
      case PieceType::silver:
        valid_dests &= attacks::silver(from, color);
        break;
      case PieceType::gold:
      case PieceType::tokin:
      case PieceType::nari_lance:
      case PieceType::nari_knight:
      case PieceType::nari_silver:
        valid_dests &= attacks::gold(from, color);
        break;
      case PieceType::king:
        // Ignore previous value of valid_dests, king can always move.
        valid_dests = attacks::king(from) & ~position.getDanger();
        break;
      case PieceType::horse:
        valid_dests &= attacks::bishop(from, position.getOccupied()) | attacks::king(from);
        break;
      case PieceType::dragon:
        valid_dests &= attacks::rook(from, position.getOccupied()) | attacks::king(from);
        break;
      }

      if (!(position.getPinned() & from_bb).empty()) {
        // We are pinned
        const Square king_sq = position.getKingSq(color);
        const Bitboard pin_ray = geometry::rayInfinite(king_sq, from);
        valid_dests &= pin_ray;
      }

      if ((valid_dests & to_bb).empty())
        return false;

      return true;
    }
  }
} // namespace lb
