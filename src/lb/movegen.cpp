#include "lb/movegen.h"

#include <bit>

#include "lb/attacks.h"
#include "lb/common.h"
#include "lb/geometry.h"
#include "lb/position.h"
#include "lb/types.h"
#include "lb/util/static_vector.h"

namespace lb::movegen {

  template <bool has_drops> static auto generateMovesTo(MoveList &moves, const Position &position, Bitboard restrict_dests) -> void;
  template <bool has_drops> static auto generateMovesNoCheckers(MoveList &moves, const Position &position, Bitboard restrict_dests) -> void;
  template <bool has_drops> static auto generateMovesOneChecker(MoveList &moves, const Position &position, Bitboard restrict_dests) -> void;
  static auto generateMovesTwoCheckers(MoveList &moves, const Position &position, Bitboard restrict_dests) -> void;

  static auto generateKingMoves(MoveList &moves, const Position &position, Bitboard restrict_dests) -> void;
  static auto generateNonKingMoves(MoveList &moves, const Position &position, Bitboard valid_dests) -> void;
  static auto generateDrops(MoveList &moves, const Position &position, Bitboard valid_dests) -> void;

  static auto splatNormalMoves(MoveList &moves, Square from, Bitboard to) -> void;
  template <PieceType ptype> static auto splatMaybePromoMoves(MoveList &moves, Color active_color, Square from, Bitboard to) -> void;
  static auto splatPromoMoves(MoveList &moves, Square from, Bitboard to) -> void;
  static auto splatDrops(MoveList &moves, PieceType ptype, Bitboard to) -> void;

  // Avoid 行き所のない駒
  auto validNormalDests(Color active_color, PieceType ptype) -> Bitboard {
    switch (ptype.raw) {
    case PieceType::none:
      lb_assert(false);
      return Bitboard{};
    case PieceType::pawn:
    case PieceType::lance:
      return ~Bitboard::rankRelative(0, active_color);
    case PieceType::knight:
      return ~(Bitboard::rankRelative(0, active_color) | Bitboard::rankRelative(1, active_color));
    default:
      return ~Bitboard{};
    }
  }

  auto generateMoves(MoveList &moves, const Position &position) -> void { generateMovesTo<true>(moves, position, ~Bitboard{}); }

  auto generateNoises(MoveList &moves, const Position &position) -> void {
    generateMovesTo<false>(moves, position, position.getColor(invert(position.activeColor())));
  }
  auto generateQuiets(MoveList &moves, const Position &position) -> void {
    generateMovesTo<true>(moves, position, ~position.getColor(invert(position.activeColor())));
  }

  template <bool has_drops> static auto generateMovesTo(MoveList &moves, const Position &position, Bitboard restrict_dests) -> void {
    switch (position.getCheckers().count()) {
    case 0:
      return generateMovesNoCheckers<has_drops>(moves, position, restrict_dests);
    case 1:
      return generateMovesOneChecker<has_drops>(moves, position, restrict_dests);
    default:
      return generateMovesTwoCheckers(moves, position, restrict_dests);
    }
  }

  template <bool has_drops> static auto generateMovesNoCheckers(MoveList &moves, const Position &position, Bitboard restrict_dests) -> void {
    const Bitboard valid_dests = ~position.getColor(position.activeColor());
    const Bitboard valid_drop_dests = ~position.getOccupied();
    generateNonKingMoves(moves, position, valid_dests & restrict_dests);
    generateKingMoves(moves, position, restrict_dests);
    if constexpr (has_drops)
      generateDrops(moves, position, valid_drop_dests & restrict_dests);
  }

  template <bool has_drops> static auto generateMovesOneChecker(MoveList &moves, const Position &position, Bitboard restrict_dests) -> void {
    const Bitboard valid_dests = geometry::rayBetween(position.getKingSq(position.activeColor()), position.getCheckers().toSq());
    generateNonKingMoves(moves, position, (valid_dests | position.getCheckers()) & restrict_dests);
    generateKingMoves(moves, position, restrict_dests);
    if constexpr (has_drops)
      generateDrops(moves, position, valid_dests & restrict_dests);
  }

  static auto generateMovesTwoCheckers(MoveList &moves, const Position &position, Bitboard restrict_dests) -> void {
    generateKingMoves(moves, position, restrict_dests);
  }

  static auto generateKingMoves(MoveList &moves, const Position &position, Bitboard restrict_dests) -> void {
    const Square king_sq = position.getKingSq(position.activeColor());
    const Bitboard king_moves = attacks::king(king_sq) & ~position.getDanger() & ~position.getColor(position.activeColor());
    splatNormalMoves(moves, king_sq, king_moves & restrict_dests);
  }

  static auto generateNonKingMoves(MoveList &moves, const Position &position, Bitboard valid_dests) -> void {
    const Color color = position.activeColor();
    const Bitboard occupied = position.getOccupied();
    const Bitboard pinned = position.getPinned();
    const Square king_sq = position.getKingSq(color);

    const auto gen = [&]<PieceType ptype>(auto op) {
      const Bitboard from_bb = ptype == PieceType::gold ? position.getPiece(color, PieceType::gold) | position.getPiece(color, PieceType::tokin) |
                                                              position.getPromoteds(color)
                                                        : position.getPiece(color, ptype);
      const Bitboard nonpinned_from_bb = from_bb & ~pinned;
      const Bitboard pinned_from_bb = from_bb & pinned;

      for (Square from : nonpinned_from_bb) {
        const Bitboard to_bb = op(from, color, occupied) & valid_dests;
        if constexpr (ptype.promotable()) {
          splatMaybePromoMoves<ptype>(moves, color, from, to_bb);
        } else {
          splatNormalMoves(moves, from, to_bb);
        }
      }

      for (Square from : pinned_from_bb) {
        const Bitboard pin_ray = geometry::rayInfinite(king_sq, from);
        const Bitboard to_bb = op(from, color, occupied) & valid_dests & pin_ray;
        if constexpr (ptype.promotable()) {
          splatMaybePromoMoves<ptype>(moves, color, from, to_bb);
        } else {
          splatNormalMoves(moves, from, to_bb);
        }
      }
    };

    gen.template operator()<PieceType::dragon>(
        [](Square from, Color color, Bitboard blockers) { return attacks::rook(from, blockers) | attacks::king(from); });
    gen.template operator()<PieceType::horse>(
        [](Square from, Color color, Bitboard blockers) { return attacks::bishop(from, blockers) | attacks::king(from); });
    gen.template operator()<PieceType::rook>([](Square from, Color color, Bitboard blockers) { return attacks::rook(from, blockers); });
    gen.template operator()<PieceType::bishop>([](Square from, Color color, Bitboard blockers) { return attacks::bishop(from, blockers); });
    gen.template operator()<PieceType::gold>([](Square from, Color color, Bitboard blockers) { return attacks::gold(from, color); });
    gen.template operator()<PieceType::silver>([](Square from, Color color, Bitboard blockers) { return attacks::silver(from, color); });
    gen.template operator()<PieceType::knight>([](Square from, Color color, Bitboard blockers) { return attacks::knight(from, color); });
    gen.template operator()<PieceType::lance>([](Square from, Color color, Bitboard blockers) { return attacks::lance(from, color, blockers); });
    gen.template operator()<PieceType::pawn>([](Square from, Color color, Bitboard blockers) { return attacks::pawn(from, color); });
  }

  static auto generateDrops(MoveList &moves, const Position &position, Bitboard valid_dests) -> void {
    const Color color = position.activeColor();

    u8 hand_ptypes = position.getHand(position.activeColor()).bithand();
    lb_assert((hand_ptypes & 1) == 0);

    // Pawn drops
    if (hand_ptypes & 2) {
      hand_ptypes &= hand_ptypes - 1;

      const Bitboard valid_normal_dests = validNormalDests(color, PieceType::pawn);
      const Bitboard nifu_restriction = position.getPiece(color, PieceType::pawn).fillFiles();
      const Bitboard enemy_king = position.getKing(invert(color));
      const Bitboard potential_uchifuzume = enemy_king.shiftRelative(Direction::n, invert(color));

      Bitboard drops = valid_dests & valid_normal_dests & ~nifu_restriction;
      if (!(drops & potential_uchifuzume).empty() && isUchifuzume(position, enemy_king.toSq(), potential_uchifuzume))
        drops &= ~potential_uchifuzume;
      splatDrops(moves, PieceType::pawn, drops);
    }

    // All other drops
    for (; hand_ptypes != 0; hand_ptypes &= hand_ptypes - 1) {
      const PieceType ptype = static_cast<PieceType::Inner>(std::countr_zero(hand_ptypes));
      const Bitboard valid_normal_dests = validNormalDests(color, ptype);
      splatDrops(moves, ptype, valid_dests & valid_normal_dests);
    }
  }

  auto isUchifuzume(const Position &position, Square enemy_king, Bitboard drop_bb) -> bool {
    const Color enemy_color = invert(position.activeColor());
    const Bitboard pawn_attackers = position.getAllNonKingAttackers(drop_bb.toSq(), enemy_color);
    const Bitboard nonpinned_pawn_attackers = pawn_attackers & ~position.getPinnedWithExtraAttackerPawns(enemy_color, drop_bb);
    if (!nonpinned_pawn_attackers.empty())
      return false;

    const Bitboard ring = attacks::king(enemy_king);
    const Bitboard attack_map = position.getAttackMapWithExtraAttackerPawns(position.activeColor(), drop_bb) | position.getColor(enemy_color);
    return (attack_map & ring) == ring;
  }

  static auto splatNormalMoves(MoveList &moves, Square from, Bitboard to_bb) -> void {
    for (Square to : to_bb) {
      moves.push_back(Move::makeMove(from, to, false));
    }
  }

  template <PieceType ptype> static auto splatMaybePromoMoves(MoveList &moves, Color active_color, Square from, Bitboard to_bb) -> void {
    const Bitboard valid_normal_dests = validNormalDests(active_color, ptype);

    if (from.isPromoSquare(active_color)) {
      splatPromoMoves(moves, from, to_bb);
      splatNormalMoves(moves, from, to_bb & valid_normal_dests);
    } else {
      splatPromoMoves(moves, from, to_bb & Bitboard::promoZone(active_color));
      splatNormalMoves(moves, from, to_bb & valid_normal_dests);
    }
  }

  static auto splatPromoMoves(MoveList &moves, Square from, Bitboard to_bb) -> void {
    for (Square to : to_bb) {
      moves.push_back(Move::makeMove(from, to, true));
    }
  }

  static auto splatDrops(MoveList &moves, PieceType ptype, Bitboard to_bb) -> void {
    for (Square to : to_bb) {
      moves.push_back(Move::makeDrop(ptype, to));
    }
  }

} // namespace lb::movegen
