#include "lb/board.h"

#include "lb/attacks.h"
#include "lb/common.h"

namespace lb {

  const Board Board::startpos = Board::parse("lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b - 1").value();

  auto Board::moveNoPrecompute(Move m) -> void {
    const usize color_index = std::to_underlying(active_color);
    const Square m_to = m.to();

    if (m.drop()) {
      const Square m_from = m.from();

      const Place src = board_mailbox[m_from.raw];
      lb_assert(src.color() == active_color && src.ptype() != PieceType::none);

      colors[color_index].clear(m_from);
      pieces[src.ptype().toBitboardIndex()].clear(m_from);
      board_mailbox[m_from.raw] = Place{};

      // Is this a capture?
      if (!board_mailbox[m_to.raw].empty()) {
        const Place captured = board_mailbox[m_to.raw];
        lb_assert(captured.color() != active_color && captured.ptype() != PieceType::none);

        colors[!color_index].clear(m_to);
        pieces[captured.ptype().toBitboardIndex()].clear(m_to);

        const PieceType hand_ptype = captured.ptype().demote();
        hand[color_index][0] |= 1 << hand_ptype.toHandIndex();
        hand[color_index][hand_ptype.toHandIndex()]++;
      }

      const PieceType dest_ptype = m.promo() ? src.ptype().promote() : src.ptype();
      colors[color_index].set(m_to);
      pieces[dest_ptype.toBitboardIndex()].set(m_to);
    } else {
      const PieceType hand_ptype = m.ptype();

      hand[color_index][0] &= ~(1 << hand_ptype.toHandIndex());
      hand[color_index][hand_ptype.toHandIndex()]--;

      colors[color_index].set(m_to);
      pieces[hand_ptype.toBitboardIndex()].set(m_to);
      board_mailbox[m_to.raw] = Place{active_color, hand_ptype};
    }

    active_color = invert(active_color);
    ply += 1;
  }

  auto Board::precompute() -> void {
    const Color friendly_color = active_color;
    const Color enemy_color = invert(active_color);

    checkers = getAllNonKingAttackers(getKingSq(friendly_color), enemy_color);
    pinned = getPinned(friendly_color);
    danger = getAttackMap(enemy_color);
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

  auto Board::getPinned(Color king_color) const -> Bitboard {
    const Color friendly_color = king_color;
    const Color enemy_color = invert(king_color);

    const Bitboard friendly_king = getKing(friendly_color);
    const Square friendly_king_sq = friendly_king.toSq();

    const Bitboard friendly = getColor(friendly_color);
    const Bitboard enemy = getColor(enemy_color);

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

  auto Board::getAttackMap(Color attacker_color) const -> Bitboard {
    const Bitboard occupied = getOccupied() & ~getKing(invert(attacker_color));

    const Bitboard orthogonals = getPiece(attacker_color, PieceType::rook) | getPiece(attacker_color, PieceType::dragon);
    const Bitboard diagonals = getPiece(attacker_color, PieceType::bishop) | getPiece(attacker_color, PieceType::horse);
    const Bitboard rings = getPiece(attacker_color, PieceType::horse) | getPiece(attacker_color, PieceType::dragon);
    const Bitboard golds = getPiece(attacker_color, PieceType::gold) | getPiece(attacker_color, PieceType::tokin) | getPromoteds(attacker_color);

    Bitboard result{};
    result |= attacks::allRooks(orthogonals, occupied);
    result |= attacks::allBishops(diagonals, occupied);
    result |= attacks::allKings(rings | getKing(attacker_color));
    result |= attacks::allGolds(golds, attacker_color);
    result |= attacks::allPawns(getPiece(attacker_color, PieceType::pawn), attacker_color);
    result |= attacks::allLances(getPiece(attacker_color, PieceType::lance), attacker_color, occupied);
    result |= attacks::allKnights(getPiece(attacker_color, PieceType::knight), attacker_color);
    result |= attacks::allSilvers(getPiece(attacker_color, PieceType::silver), attacker_color);
    return result;
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
        } else if (const usize pt = PieceType::piece_order_sente.find(ch); pt != std::string_view::npos) {
          result.placeBoardFromParse(Color::sente, static_cast<PieceType::Inner>(pt + 1), sq);
          place_index++;
        } else if (const usize pt = PieceType::piece_order_gote.find(ch); pt != std::string_view::npos) {
          result.placeBoardFromParse(Color::gote, static_cast<PieceType::Inner>(pt + 1), sq);
          place_index++;
        } else if (ch == '+') {
          i++;
          if (i >= board_str.size())
            return std::unexpected(ParseError::invalid_length);
          const char ch2 = board_str[i];
          if (const usize pt = PieceType::piece_order_sente.find(ch2); pt != std::string_view::npos) {
            PieceType ptype = static_cast<PieceType::Inner>(pt + 1);
            if (!ptype.promotable())
              return std::unexpected(ParseError::invalid_char);
            result.placeBoardFromParse(Color::sente, ptype.promote(), sq);
          } else if (const usize pt = PieceType::piece_order_gote.find(ch2); pt != std::string_view::npos) {
            PieceType ptype = static_cast<PieceType::Inner>(pt + 1);
            if (!ptype.promotable())
              return std::unexpected(ParseError::invalid_char);
            result.placeBoardFromParse(Color::gote, ptype.promote(), sq);
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
        } else if (const usize pt = PieceType::piece_order_sente.find(ch); pt != std::string_view::npos && ch != 'K') {
          if (!result.placeHandFromParse(Color::sente, static_cast<PieceType::Inner>(pt + 1), modifier.value_or(1)))
            return std::unexpected(ParseError::invalid_hand);
          modifier = std::nullopt;
        } else if (const usize pt = PieceType::piece_order_gote.find(ch); pt != std::string_view::npos && ch != 'k') {
          if (!result.placeHandFromParse(Color::gote, static_cast<PieceType::Inner>(pt + 1), modifier.value_or(1)))
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

    result.precompute();

    return result;
  }

} // namespace lb
