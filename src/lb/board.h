#pragma once

#include <array>
#include <format>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>

#include "lb/common.h"
#include "lb/types.h"
#include "lb/util/tokenizer.h"
#include "lb/zhash.h"

namespace lb {

  struct Board;

  struct Place {
    u8 raw = 0;

    constexpr Place() = default;
    constexpr Place(Color color, PieceType ptype) : raw((std::to_underlying(color) << 7) | std::to_underlying(ptype.raw)) {}

    inline constexpr auto empty() const -> bool { return raw == 0; }
    inline constexpr auto color() const -> Color { return static_cast<Color>(raw >> 7); }
    inline constexpr auto ptype() const -> PieceType { return static_cast<PieceType::Inner>(raw & 0x7F); }

    inline constexpr auto operator==(const Place &) const -> bool = default;
  };

  struct Board {
  private:
    friend class std::formatter<lb::Board, char>;

    using Hand = std::array<u8, 8>;

    std::array<Bitboard, 2> colors{};
    std::array<Bitboard, PieceType::bitboard_count> pieces{};
    std::array<Place, 81> board_mailbox{};
    std::array<Hand, 2> hand{};
    Color active_color{};
    u16 ply{};

    // Number of two-plys ago color was not checked.
    //     if non_check_clock[0] == 0, then gote's most recent move did not check sente
    //     if non_check_clock[0] == 1, gote's most recent move checked sente.
    //     if non_check_clock[0] == 2 and active_color == sente, then previous moves were:
    //         [gote checked sente] [sente move] [gote checked sente] <sente to move>
    std::array<u16, 2> non_check_clock{};

    Bitboard checkers{};
    Bitboard pinned{};
    Bitboard danger{};

    zhash::Hash hash{};

  public:
    static const Board startpos;

    constexpr Board() = default;

    inline constexpr auto activeColor() const -> Color { return active_color; }
    inline constexpr auto nonCheckClock(Color color) const -> u16 { return non_check_clock[std::to_underlying(color)]; }

    inline constexpr auto getColor(Color color) const -> Bitboard { return colors[std::to_underlying(color)]; }
    inline constexpr auto getOccupied() const -> Bitboard { return colors[0] | colors[1]; }
    inline constexpr auto getPiece(Color color, PieceType ptype) const -> Bitboard {
      return colors[std::to_underlying(color)] & pieces[ptype.toBitboardIndex()];
    }
    inline constexpr auto getKing(Color color) const -> Bitboard { return getPiece(color, PieceType::king); }
    inline constexpr auto getKingSq(Color color) const -> Square { return getKing(color).toSq(); }
    inline constexpr auto getPromoteds(Color color) const -> Bitboard { return colors[std::to_underlying(color)] & pieces.back(); }

    inline constexpr auto getPlace(Square sq) const -> Place { return board_mailbox[sq.raw]; }
    inline constexpr auto getHand(Color color) const -> Hand { return hand[std::to_underlying(color)]; }

    inline constexpr auto isInCheck() const -> bool { return !checkers.empty(); }
    inline constexpr auto getCheckers() const -> Bitboard { return checkers; }
    inline constexpr auto getPinned() const -> Bitboard { return pinned; }
    inline constexpr auto getDanger() const -> Bitboard { return danger; }
    inline constexpr auto getHash() const -> zhash::Hash { return hash; }

    inline auto move(Move m) const -> Board {
      Board result = *this;
      result.moveNoPrecompute(m);
      result.precompute();
      return result;
    }

    auto moveNoPrecompute(Move m) -> void;
    auto precompute() -> void;

    auto getAllNonKingAttackers(Square sq, Color attacker_color) const -> Bitboard;
    auto getPinned(Color king_color) const -> Bitboard;
    auto getPinnedWithExtraAttackerPawns(Color king_color, Bitboard extra_pawns) const -> Bitboard;
    auto getAttackMap(Color attacker_color) const -> Bitboard;
    auto getAttackMapWithExtraAttackerPawns(Color attacker_color, Bitboard extra_pawns) const -> Bitboard;

    auto calcHashSlow() const -> zhash::Hash;

    auto printKifu() const -> void;

    static auto parse(std::string_view str) -> std::expected<Board, ParseError> {
      Tokenizer it{str};
      const std::string_view board_str = it.next();
      const std::string_view color = it.next();
      const std::string_view hand = it.next();
      const std::string_view ply = it.next();
      if (!it.rest().empty())
        return std::unexpected(ParseError::invalid_length);
      return parse(board_str, color, hand, ply);
    }

    static auto parse(std::string_view board_str, std::string_view color_str, std::string_view hand_str, std::string_view ply_str)
        -> std::expected<Board, ParseError>;

    constexpr auto operator==(const Board &) const -> bool = default;

  private:
    auto placeBoardFromParse(Color color, PieceType ptype, Square sq) -> void;
    auto placeHandFromParse(Color color, PieceType ptype, usize count) -> bool;
  };

} // namespace lb

template <> struct std::formatter<lb::Board, char> {
  template <class ParseContext> constexpr auto parse(ParseContext &ctx) -> ParseContext::iterator { return ctx.begin(); }

  template <class FmtContext> auto format(const lb::Board &board, FmtContext &ctx) const -> FmtContext::iterator {
    using namespace lb;
    usize blanks = 0;
    for (usize place_index : std::views::iota(0, 81)) {
      const usize file = 8 - place_index % 9;
      const usize rank = place_index / 9;
      const Square sq = Square::fromFileAndRank(file, rank);
      const Place place = board.board_mailbox[sq.raw];
      if (place.ptype() == PieceType::none) {
        blanks++;
      } else {
        if (blanks != 0) {
          std::format_to(ctx.out(), "{}", blanks);
          blanks = 0;
        }
        std::format_to(ctx.out(), "{}", place.ptype().toEnString(place.color()));
      }
      if (file == 0) {
        if (blanks != 0) {
          std::format_to(ctx.out(), "{}", blanks);
          blanks = 0;
        }
        if (place_index != 80)
          std::format_to(ctx.out(), "/");
      }
    }
    std::format_to(ctx.out(), " {} ", board.active_color);
    if (board.hand[0][0] == 0 && board.hand[1][0] == 0) {
      std::format_to(ctx.out(), "-");
    } else {
      for (usize c : std::views::iota(0, 2)) {
        using PT = PieceType;
        for (PieceType ptype : {PT::rook, PT::bishop, PT::gold, PT::silver, PT::knight, PT::lance, PT::pawn}) {
          const usize count = board.hand[c][ptype.toHandIndex()];
          if (count > 0) {
            if (count > 1)
              std::format_to(ctx.out(), "{}", count);
            std::format_to(ctx.out(), "{}", ptype.toEnString(static_cast<Color>(c)));
          }
        }
      }
    }
    return std::format_to(ctx.out(), " {}", board.ply + 1);
  }
};
