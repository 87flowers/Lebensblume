#include "lb/board.h"

#include <print>
#include <ranges>
#include <utility>

#include "lb/attacks.h"
#include "lb/common.h"
#include "lb/numbers.h"
#include "lb/zhash.h"

namespace lb {

  const Board Board::startpos = Board::parse("lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b - 1").value();

  auto Board::moveNoPrecompute(Move m) -> void {
    const usize color_index = std::to_underlying(active_color);
    const Square m_to = m.to();

    if (m.drop()) {
      const PieceType hand_ptype = m.ptype();

      const u8 was_in_hand = getHand(active_color).removePiece(hand_ptype) + 1;

      hash ^= zhash::handIncremental(active_color, hand_ptype, was_in_hand);

      colors[color_index].set(m_to);
      pieces[hand_ptype.toBitboardIndex()].set(m_to);
      board_mailbox[m_to.raw] = Place{active_color, hand_ptype};

      hash ^= zhash::board(active_color, hand_ptype, m_to);
    } else {
      const Square m_from = m.from();

      const Place src = board_mailbox[m_from.raw];
      lb_assert(src.color() == active_color && src.ptype() != PieceType::none);

      colors[color_index].clear(m_from);
      pieces[src.ptype().toBitboardIndex()].clear(m_from);
      board_mailbox[m_from.raw] = Place{};

      hash ^= zhash::board(active_color, src.ptype(), m_from);

      // Is this a capture?
      if (!board_mailbox[m_to.raw].empty()) {
        const Place captured = board_mailbox[m_to.raw];
        lb_assert(captured.color() != active_color && captured.ptype() != PieceType::none);

        colors[!color_index].clear(m_to);
        pieces[captured.ptype().toBitboardIndex()].clear(m_to);

        hash ^= zhash::board(captured.color(), captured.ptype(), m_to);

        const PieceType hand_ptype = captured.ptype().demote();
        const u8 now_in_hand = getHand(active_color).addPiece(hand_ptype);

        hash ^= zhash::handIncremental(active_color, hand_ptype, now_in_hand);
      }

      const PieceType dest_ptype = m.promo() ? src.ptype().promote() : src.ptype();
      colors[color_index].set(m_to);
      pieces[dest_ptype.toBitboardIndex()].set(m_to);
      board_mailbox[m_to.raw] = Place{active_color, dest_ptype};

      hash ^= zhash::board(active_color, dest_ptype, m_to);
    }

    active_color = invert(active_color);
    hash ^= zhash::move;
    ply += 1;

    checkers = getAllNonKingAttackers(getKingSq(active_color), invert(active_color));
    if (checkers.empty()) {
      non_check_clock[std::to_underlying(active_color)] = 0;
    } else {
      non_check_clock[std::to_underlying(active_color)] += 1;
    }

    lb_assert(hash == calcHashSlow(), "{} {} {:x} {:x} {:x}", *this, m, hash, calcHashSlow(), hash ^ calcHashSlow());
  }

  auto Board::precompute() -> void {
    const Color friendly_color = active_color;
    const Color enemy_color = invert(active_color);

    pinned = getPinned(friendly_color);
    danger = getAttackMap(enemy_color);
  }

  // 入玉宣言法
  auto Board::canDeclareEnteringKingsWin() const -> bool {
    // Based on WCSC rules (dated 2023-11-27):
    // The program may declare a win (such a declaration being called “declaration of a win”) if the position satisfies all of the
    // following conditions.  If the position does not satisfy one or more conditions, then the declaring side loses:
    // 次の各号に掲げる条件がすべて成立する場合、勝ちを宣言できる（以下「入玉宣言」という）。１つでも条件を満たしていない場合、宣言した方が負けとなる。

    // 1. It is the declaring side's turn.
    // 一 宣言側の手番である
    // 6. The declaring side has at least one second left.
    // 六 宣言側の持ち時間が残っている

    // 5. There is no check on the King of the declaring side.
    // 五 宣言側の玉に王手がかかっていない。
    if (isInCheck())
      return false;

    const Bitboard promo_zone = Bitboard::promoZone(active_color);

    // 2. The King of the declaring side is in the third rank or beyond.
    // 二 宣言側の玉が敵陣三段目以内に入っている。
    if ((getKing(active_color) & promo_zone).empty())
      return false;

    // 4. The declaring side has 10 or more pieces other than the King in the third rank or beyond.
    // 四 宣言側の敵陣三段目以内の駒は、玉を除いて１０枚以上存在する。
    const Bitboard in_promo_zone = getColor(active_color) & promo_zone;
    const usize num_in_promo_zone = in_promo_zone.count() - 1; // Exclude king
    if (num_in_promo_zone < 10)
      return false;

    const Bitboard board_bigs = getPiece(active_color, PieceType::rook) | getPiece(active_color, PieceType::bishop) |
                                getPiece(active_color, PieceType::dragon) | getPiece(active_color, PieceType::horse);
    const usize board_piece_points = num_in_promo_zone + 4 * (board_bigs & promo_zone).count();

    const Hand hand = getHand(active_color);
    const usize hand_piece_points = hand.getPiece(PieceType::pawn) + hand.getPiece(PieceType::lance) + hand.getPiece(PieceType::knight) +
                                    hand.getPiece(PieceType::silver) + hand.getPiece(PieceType::gold) + 5 * hand.getPiece(PieceType::bishop) +
                                    5 * hand.getPiece(PieceType::rook);

    const usize piece_points = board_piece_points + hand_piece_points;

    // 3. The declaring side has 28 (the first player) or 27 (the second player) piece points or more.
    //    Piece points are counted only for pieces of the declaring side that are in hand or in the third rank or beyond.
    //    Piece points are counted as follows:  King: 0; Rook, Bishop, Promoted Rook, or Promoted Bishop: 5; Other: 1.
    // 三 宣言側が、大駒５点小駒１点で計算して
    // 　・先手の場合２８点以上の持点がある。
    // 　・後手の場合２７点以上の持点がある。
    // 　・点数の対象となるのは、宣言側の持駒と敵陣三段目以内に存在する玉を除く宣言側の駒のみである。
    switch (active_color) {
    case Color::sente:
      return piece_points >= 28;
    case Color::gote:
      return piece_points >= 27;
    }
  }

  auto Board::getAllNonKingAttackers(Square sq, Color attacker_color) const -> Bitboard {
    const Color defender_color = invert(attacker_color);
    const Bitboard occupied = getOccupied();

    const Bitboard orthogonals = getPiece(attacker_color, PieceType::rook) | getPiece(attacker_color, PieceType::dragon);
    const Bitboard diagonals = getPiece(attacker_color, PieceType::bishop) | getPiece(attacker_color, PieceType::horse);
    const Bitboard rings = getPiece(attacker_color, PieceType::horse) | getPiece(attacker_color, PieceType::dragon);
    const Bitboard golds = getPiece(attacker_color, PieceType::gold) | getPiece(attacker_color, PieceType::tokin) | getPromoteds(attacker_color);

    Bitboard result{};
    result |= attacks::rook(sq, occupied) & orthogonals;
    result |= attacks::bishop(sq, occupied) & diagonals;
    result |= attacks::king(sq) & rings;
    result |= attacks::gold(sq, defender_color) & golds;
    result |= attacks::pawn(sq, defender_color) & getPiece(attacker_color, PieceType::pawn);
    result |= attacks::lance(sq, defender_color, occupied) & getPiece(attacker_color, PieceType::lance);
    result |= attacks::knight(sq, defender_color) & getPiece(attacker_color, PieceType::knight);
    result |= attacks::silver(sq, defender_color) & getPiece(attacker_color, PieceType::silver);
    return result;
  }

  auto Board::getPinned(Color king_color) const -> Bitboard { return getPinnedWithExtraAttackerPawns(king_color, Bitboard{}); }

  auto Board::getPinnedWithExtraAttackerPawns(Color king_color, Bitboard extra_pawns) const -> Bitboard {
    const Color friendly_color = king_color;
    const Color enemy_color = invert(king_color);

    const Bitboard friendly_king = getKing(friendly_color);
    const Square friendly_king_sq = friendly_king.toSq();

    const Bitboard friendly = getColor(friendly_color);
    const Bitboard enemy = getColor(enemy_color) | extra_pawns;

    const Bitboard orthogonals = getPiece(enemy_color, PieceType::rook) | getPiece(enemy_color, PieceType::dragon);
    const Bitboard diagonals = getPiece(enemy_color, PieceType::bishop) | getPiece(enemy_color, PieceType::horse);

    const Bitboard orthogonal_rays = attacks::rook(friendly_king_sq, enemy);
    const Bitboard diagonal_rays = attacks::bishop(friendly_king_sq, enemy);

    const Bitboard lance_pinners = attacks::lance(friendly_king_sq, friendly_color, enemy) & getPiece(enemy_color, PieceType::lance);
    const Bitboard orthogonal_pinners = (orthogonal_rays & orthogonals) | lance_pinners;
    const Bitboard diagonal_pinners = diagonal_rays & diagonals;

    Bitboard result{};
    for (Square pinner : orthogonal_pinners) {
      const Bitboard ray = orthogonal_rays & attacks::rook(pinner, friendly_king);
      const Bitboard potential_pinned = ray & friendly;
      if (potential_pinned.count() == 1)
        result |= potential_pinned;
    }
    for (Square pinner : diagonal_pinners) {
      const Bitboard ray = diagonal_rays & attacks::bishop(pinner, friendly_king);
      const Bitboard potential_pinned = ray & friendly;
      if (potential_pinned.count() == 1)
        result |= potential_pinned;
    }
    return result;
  }

  auto Board::getAttackMap(Color attacker_color) const -> Bitboard { return getAttackMapWithExtraAttackerPawns(attacker_color, Bitboard{}); }

  auto Board::getAttackMapWithExtraAttackerPawns(Color attacker_color, Bitboard extra_pawns) const -> Bitboard {
    const Bitboard occupied = (getOccupied() & ~getKing(invert(attacker_color))) | extra_pawns;

    const Bitboard orthogonals = getPiece(attacker_color, PieceType::rook) | getPiece(attacker_color, PieceType::dragon);
    const Bitboard diagonals = getPiece(attacker_color, PieceType::bishop) | getPiece(attacker_color, PieceType::horse);
    const Bitboard rings = getPiece(attacker_color, PieceType::horse) | getPiece(attacker_color, PieceType::dragon);
    const Bitboard golds = getPiece(attacker_color, PieceType::gold) | getPiece(attacker_color, PieceType::tokin) | getPromoteds(attacker_color);

    Bitboard result{};
    result |= attacks::allRooks(orthogonals, occupied);
    result |= attacks::allBishops(diagonals, occupied);
    result |= attacks::allKings(rings | getKing(attacker_color));
    result |= attacks::allGolds(golds, attacker_color);
    result |= attacks::allPawns(getPiece(attacker_color, PieceType::pawn) | extra_pawns, attacker_color);
    result |= attacks::allLances(getPiece(attacker_color, PieceType::lance), attacker_color, occupied);
    result |= attacks::allKnights(getPiece(attacker_color, PieceType::knight), attacker_color);
    result |= attacks::allSilvers(getPiece(attacker_color, PieceType::silver), attacker_color);
    return result;
  }

  auto Board::calcHashSlow() const -> zhash::Hash {
    zhash::Hash result = 0;
    for (u8 i : std::views::iota(0, 81)) {
      const Square sq{i};
      const Place &place = board_mailbox[i];
      if (!place.empty()) {
        result ^= zhash::board(place.color(), place.ptype(), sq);
      }
    }
    for (usize c : std::views::iota(0, 2)) {
      for (usize pt : std::views::iota(1, 8)) {
        const Color color = static_cast<Color>(c);
        const PieceType ptype = static_cast<PieceType::Inner>(pt);
        const usize count = getHand(color).getPiece(ptype);
        result ^= zhash::hand(color, ptype, count);
      }
    }
    if (active_color == Color::gote) {
      result ^= zhash::move;
    }
    return result;
  }

  auto Board::printKifu() const -> void {
    using PT = PieceType;

    const auto print_hand = [this](const Hand &hand) {
      if (hand.bithand() == 0) {
        std::print("なし");
      } else {
        bool first = true;
        for (PieceType ptype : {PT::rook, PT::bishop, PT::gold, PT::silver, PT::knight, PT::lance, PT::pawn}) {
          const usize count = hand.getPiece(ptype);
          if (count > 0) {
            std::print("{}{}{}", first ? "" : " ", ptype.toJaString(), count > 1 ? numbers::kanji_table[count] : "");
            first = false;
          }
        }
      }
      std::print("\n");
    };

    std::print("後手の持駒：");
    print_hand(getHand(Color::gote));
    std::print("  ９ ８ ７ ６ ５ ４ ３ ２ １\n");
    std::print("+---------------------------+\n");
    for (i8 rank = 0; rank < 9; rank++) {
      std::print("|");
      for (i8 file = 8; file >= 0; file--) {
        const Square sq = Square::fromFileAndRank(static_cast<usize>(file), static_cast<usize>(rank));
        const Place place = board_mailbox[sq.raw];
        std::print("{}{}", place.color() == Color::gote ? "v" : " ", place.ptype().toJaString());
      }
      std::print("|{}\n", numbers::kanji_table[rank + 1]);
    }
    std::print("+---------------------------+\n");
    std::print("先手の持駒：");
    print_hand(getHand(Color::sente));
  }

  auto Board::parse(std::string_view board_str, std::string_view color_str, std::string_view hand_str, std::string_view ply_str)
      -> std::expected<Board, ParseError> {
    Board result{};

    // Parse board
    {
      usize place_index = 0, i = 0;
      for (; place_index < 81 && i < board_str.size(); i++) {
        const usize file = 8 - place_index % 9;
        const usize rank = place_index / 9;
        const Square sq = Square::fromFileAndRank(file, rank);
        const char ch = board_str[i];
        if (ch == '/') {
          if (file != 8 || place_index == 0)
            return std::unexpected(ParseError::invalid_char);
        } else if (ch >= '1' and ch <= '9') {
          const usize spaces = ch - '0';
          if (spaces > file + 1)
            return std::unexpected(ParseError::invalid_char);
          place_index += spaces;
        } else if (const auto ptype = PieceType::parseSente(ch); ptype.has_value()) {
          result.placeBoardFromParse(Color::sente, ptype.value(), sq);
          place_index++;
        } else if (const auto ptype = PieceType::parseGote(ch); ptype.has_value()) {
          result.placeBoardFromParse(Color::gote, ptype.value(), sq);
          place_index++;
        } else if (ch == '+') {
          i++;
          if (i >= board_str.size())
            return std::unexpected(ParseError::invalid_length);
          const char ch2 = board_str[i];
          if (const auto ptype = PieceType::parseSente(ch2); ptype.has_value()) {
            if (!ptype.value().promotable())
              return std::unexpected(ParseError::invalid_char);
            result.placeBoardFromParse(Color::sente, ptype.value().promote(), sq);
          } else if (const auto ptype = PieceType::parseGote(ch2); ptype.has_value()) {
            if (!ptype.value().promotable())
              return std::unexpected(ParseError::invalid_char);
            result.placeBoardFromParse(Color::gote, ptype.value().promote(), sq);
          } else {
            return std::unexpected(ParseError::invalid_char);
          }
          place_index++;
        } else {
          return std::unexpected(ParseError::invalid_char);
        }
      }
      if (place_index != 81 || i != board_str.size())
        return std::unexpected(ParseError::invalid_length);
    }

    // Parse Color
    {
      if (color_str.size() != 1)
        return std::unexpected(ParseError::invalid_length);
      switch (color_str[0]) {
      case 'b':
        result.active_color = Color::sente;
        break;
      case 'w':
        result.active_color = Color::gote;
        break;
      default:
        return std::unexpected(ParseError::invalid_char);
      }
    }

    // Parse Hand
    if (hand_str != "-") {
      std::optional<usize> modifier = std::nullopt;
      for (char ch : hand_str) {
        if (ch >= '0' and ch <= '9') {
          if (!modifier and ch == '0')
            return std::unexpected(ParseError::invalid_char);
          modifier = modifier.value_or(0) * 10 + (ch - '0');
          if (modifier.value() > 18)
            return std::unexpected(ParseError::out_of_range);
        } else if (const auto ptype = PieceType::parseSente(ch); ptype && ptype != PieceType::king) {
          if (!result.placeHandFromParse(Color::sente, ptype.value(), modifier.value_or(1)))
            return std::unexpected(ParseError::invalid_hand);
          modifier = std::nullopt;
        } else if (const auto ptype = PieceType::parseGote(ch); ptype && ptype != PieceType::king) {
          if (!result.placeHandFromParse(Color::gote, ptype.value(), modifier.value_or(1)))
            return std::unexpected(ParseError::invalid_hand);
          modifier = std::nullopt;
        } else {
          return std::unexpected(ParseError::invalid_char);
        }
      }
    }

    // Parse ply
    if (ply_str.empty()) {
      result.ply = std::to_underlying(result.active_color);
    } else if (const usize ply = std::stoi(std::string{ply_str}); ply != 0 && ply < 10000) {
      result.ply = ply - 1;
    } else {
      return std::unexpected(ParseError::out_of_range);
    }

    // King count validation
    if (result.getKing(Color::sente).count() != 1 || result.getKing(Color::gote).count() != 1)
      return std::unexpected(ParseError::too_many_kings);

    result.checkers = result.getAllNonKingAttackers(result.getKingSq(result.active_color), invert(result.active_color));
    result.precompute();
    result.hash = result.calcHashSlow();

    result.non_check_clock = {};
    if (!result.checkers.empty()) {
      result.non_check_clock[std::to_underlying(result.active_color)] = 1;
    }

    return result;
  }

  auto Board::placeBoardFromParse(Color color, PieceType ptype, Square sq) -> void {
    const Bitboard bb = Bitboard::fromSq(sq);
    colors[std::to_underlying(color)] |= bb;
    pieces[ptype.toBitboardIndex()] |= bb;
    board_mailbox[sq.raw] = Place{color, ptype};
  }

  auto Board::placeHandFromParse(Color color, PieceType ptype, usize count) -> bool {
    const std::array<usize, 8> max_count{{0, 18, 2, 2, 4, 4, 4, 4}};
    if (count > max_count[ptype.toHandIndex()] || count == 0)
      return false;
    if (getHand(color).getPiece(ptype) != 0)
      return false;
    getHand(color).setPiece(ptype, static_cast<u8>(count));
    return true;
  }

} // namespace lb
