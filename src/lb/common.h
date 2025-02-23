#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <compare>
#include <expected>
#include <format>
#include <string_view>
#include <utility>

#include "lb/types.h"
#include "lb/util/assert.h"

namespace lb {
  constexpr usize max_legal_moves = 600;
  constexpr usize max_search_ply = 128;

  enum class ParseError {
    invalid_char,
    invalid_length,
    out_of_range,
    invalid_hand,
    invalid_board,
    too_many_kings,
  };

  enum class Color {
    sente = 0,
    gote = 1,
  };

  inline constexpr auto invert(Color c) -> Color { return c == Color::sente ? Color::gote : Color::sente; }

  struct PieceType {
    enum Inner : u8 {
      none = 0x0,
      pawn = 0x1,
      bishop = 0x2,
      rook = 0x3,
      lance = 0x4,
      knight = 0x5,
      silver = 0x6,
      gold = 0x7,
      king = 0x8,
      tokin = 0x9,
      horse = 0xA,
      dragon = 0xB,
      nari_lance = 0xC,
      nari_knight = 0xD,
      nari_silver = 0xE,
    };

    Inner raw = none;

    static constexpr size_t bitboard_count = 0xC;

    /* implicit */ constexpr PieceType(Inner raw) : raw(raw) {}

    inline constexpr auto promotable() const -> bool { return raw >= pawn && raw <= silver; }
    inline constexpr auto promoted() const -> bool { return raw >= tokin; }
    inline constexpr auto promote() const -> PieceType {
      lb_assert(raw != none && raw != gold);
      return PieceType{static_cast<Inner>(raw | 0x8)};
    }
    inline constexpr auto demote() const -> PieceType { return promoted() ? PieceType{static_cast<Inner>(raw & 0x7)} : *this; }
    inline constexpr auto toBitboardIndex() const -> usize {
      lb_assert(raw != none);
      return std::min<usize>(bitboard_count, raw) - 1;
    }
    inline constexpr auto toHandIndex() const -> usize {
      lb_assert(raw != none && raw < king);
      return static_cast<usize>(raw);
    }

    inline constexpr auto operator==(const PieceType &) const -> bool = default;

    inline static constexpr std::array<std::array<const char *, 15>, 2> en_strings{{
        {"-", "P", "B", "R", "L", "N", "S", "G", "K", "+P", "+B", "+R", "+L", "+N", "+S"},
        {"-", "p", "b", "r", "l", "n", "s", "g", "k", "+p", "+b", "+r", "+l", "+n", "+s"},
    }};
    inline static constexpr std::array<std::array<const char *, 15>, 2> ja_strings{{
        {"　", "歩", "角", "飛", "香", "桂", "銀", "金", "玉", "と", "馬", "龍", "杏", "圭", "全"},
        {"　", "歩", "角", "飛", "香", "桂", "銀", "金", "王", "と", "馬", "龍", "杏", "圭", "全"},
    }};

    static constexpr auto parseSente(char ch) -> std::expected<PieceType, ParseError> {
      constexpr std::string_view piece_order_sente{"PBRLNSGK"};
      const usize pt = piece_order_sente.find(ch);
      if (pt == std::string_view::npos)
        return std::unexpected(ParseError::invalid_char);
      return static_cast<Inner>(pt + 1);
    }

    static constexpr auto parseGote(char ch) -> std::expected<PieceType, ParseError> {
      constexpr std::string_view piece_order_gote{"pbrlnsgk"};
      const usize pt = piece_order_gote.find(ch);
      if (pt == std::string_view::npos)
        return std::unexpected(ParseError::invalid_char);
      return static_cast<Inner>(pt + 1);
    }
  };

  struct Square {
    u8 raw = 0;

    inline explicit constexpr Square(u8 raw) : raw(raw) { lb_assert(raw < 81); }

    inline static constexpr auto fromFileAndRank(usize file, usize rank) -> Square { return Square{narrow_cast<u8>(rank * 9 + file)}; }

    inline constexpr auto isPromoSquare(Color color) const -> bool {
      switch (color) {
      case Color::sente:
        return raw < 3 * 9;
      case Color::gote:
        return raw >= 6 * 9;
      }
      std::unreachable();
    }

    static constexpr auto parse(std::string_view str) -> std::expected<Square, ParseError> {
      if (str.size() != 2)
        return std::unexpected(ParseError::invalid_length);
      if (str[0] < '1' or str[0] > '9')
        return std::unexpected(ParseError::invalid_char);
      const u8 file = str[0] - '1';
      if (str[1] < 'a' or str[1] > 'i')
        return std::unexpected(ParseError::invalid_char);
      const u8 rank = str[1] - 'a';
      return fromFileAndRank(file, rank);
    }

    constexpr auto operator<=>(const Square &) const -> std::strong_ordering = default;
  };

  struct Move {
    u16 raw = 0;

    constexpr Move() = default;
    explicit constexpr Move(u16 raw) : raw(raw) {}

    static constexpr Move none() { return Move{0}; };

    static constexpr auto makeMove(Square from, Square to, bool promotion) -> Move {
      u16 result = 0;
      result |= to.raw;
      result |= static_cast<u16>(from.raw) << 8;
      result |= static_cast<u16>(promotion) << 15;
      return Move{result};
    }

    static constexpr auto makeDrop(PieceType ptype, Square to) -> Move {
      u16 result = drop_flag;
      result |= to.raw;
      result |= static_cast<u16>(ptype.raw) << 8;
      return Move{result};
    }
    constexpr auto drop() const -> bool { return (raw & drop_flag) != 0; }
    constexpr auto to() const -> Square { return Square{static_cast<u8>(raw & 0x7F)}; }
    constexpr auto promo() const -> bool {
      lb_assert(!drop());
      return static_cast<bool>(raw >> 15);
    }
    constexpr auto from() const -> Square {
      lb_assert(!drop());
      return Square{static_cast<u8>((raw >> 8) & 0x7F)};
    }
    constexpr auto ptype() const -> PieceType {
      lb_assert(drop());
      return PieceType{static_cast<PieceType::Inner>(raw >> 8)};
    }

    static constexpr auto parse(std::string_view str) -> std::expected<Move, ParseError> {
      if (str.size() != 4 && str.size() != 5)
        return std::unexpected(ParseError::invalid_length);

      const auto to = Square::parse(str.substr(2, 2));
      if (!to)
        return std::unexpected(to.error());

      if (str[1] == '*') {
        if (str.size() != 4)
          return std::unexpected(ParseError::invalid_length);

        const auto ptype = PieceType::parseSente(str[0]);
        if (!ptype || ptype.value() == PieceType::king)
          return std::unexpected(ParseError::invalid_char);

        return makeDrop(ptype.value(), to.value());
      }

      const bool promo = str.size() == 5;
      if (promo && str[4] != '+')
        return std::unexpected(ParseError::invalid_char);

      const auto from = Square::parse(str.substr(0, 2));
      if (!from)
        return std::unexpected(from.error());

      return makeMove(from.value(), to.value(), promo);
    }

    inline constexpr auto operator==(const Move &) const -> bool = default;

  private:
    inline static constexpr u16 drop_flag = 1 << 7;
  };

  enum class Direction { n = 0, ne = 1, e = 2, se = 3, s = 4, sw = 5, w = 6, nw = 7 };

  struct Bitboard {
    u128 raw = 0;

    constexpr Bitboard() = default;
    explicit constexpr Bitboard(u128 raw) : raw(raw) {}

    inline static constexpr auto rank(usize i) -> Bitboard { return Bitboard{rank_mask << (i * 9)}; }
    inline static constexpr auto file(usize i) -> Bitboard { return Bitboard{file_mask << i}; }
    inline static constexpr auto rankRelative(usize i, Color perspective) -> Bitboard {
      switch (perspective) {
      case Color::sente:
        return rank(i);
      case Color::gote:
        return rank(8 - i);
      }
      std::unreachable();
    }
    inline static constexpr auto promoZone(Color perspective) -> Bitboard {
      constexpr Bitboard sente_promo_zone{rank(0).raw | rank(1).raw | rank(2).raw};
      constexpr Bitboard gote_promo_zone{rank(6).raw | rank(7).raw | rank(8).raw};
      switch (perspective) {
      case Color::sente:
        return sente_promo_zone;
      case Color::gote:
        return gote_promo_zone;
      }
      std::unreachable();
    }

    static constexpr auto fromSq(Square sq) -> Bitboard { return Bitboard{1_u128 << sq.raw}; }
    constexpr auto toSq() const -> Square { return Square{narrow_cast<u8>(std::countr_zero(raw))}; }

    inline constexpr auto empty() const -> bool { return raw == 0; }
    inline constexpr auto count() const -> usize { return std::popcount(static_cast<u64>(raw)) + std::popcount(static_cast<u64>(raw >> 64)); }

    inline constexpr auto clear(Square sq) -> void { raw &= ~fromSq(sq).raw; }
    inline constexpr auto set(Square sq) -> void { raw |= fromSq(sq).raw; }

    constexpr auto fillFiles() const -> Bitboard {
      u128 up = raw;
      u128 down = raw;
      up |= up << 9;
      down |= down >> 9;
      up |= up << 18;
      down |= down >> 18;
      up |= up << 36;
      down |= down >> 36;
      up |= up << 72;
      down |= down >> 72;
      return Bitboard{(up | down) & mask};
    }

    inline constexpr auto shift(Direction dir) const -> Bitboard {
      switch (dir) {
      case Direction::n:
        return Bitboard{raw >> 9};
      case Direction::s:
        return Bitboard{(raw << 9) & mask};
      case Direction::e:
        return Bitboard{(raw & ~file(0).raw) >> 1};
      case Direction::w:
        return Bitboard{((raw & ~file(8).raw) << 1) & mask};
      case Direction::ne:
        return Bitboard{(raw & ~file(0).raw) >> 10};
      case Direction::nw:
        return Bitboard{(raw & ~file(8).raw) >> 8};
      case Direction::se:
        return Bitboard{((raw & ~file(0).raw) << 8) & mask};
      case Direction::sw:
        return Bitboard{((raw & ~file(8).raw) << 10) & mask};
      }
      std::unreachable();
    }

    inline constexpr auto shiftRelative(Direction dir, Color perspective) const -> Bitboard {
      switch (perspective) {
      case Color::sente:
        return shift(dir);
      case Color::gote:
        return shift(static_cast<Direction>((static_cast<usize>(dir) + 4) % 8));
      }
      std::unreachable();
    }

    inline constexpr auto operator~() const -> Bitboard { return Bitboard{~raw & mask}; }

    constexpr auto operator|=(Bitboard b) -> Bitboard & {
      raw |= b.raw;
      return *this;
    }

    constexpr auto operator^=(Bitboard b) -> Bitboard & {
      raw ^= b.raw;
      return *this;
    }

    constexpr auto operator&=(Bitboard b) -> Bitboard & {
      raw &= b.raw;
      return *this;
    }

    inline auto operator==(const Bitboard &) const -> bool = default;

    struct Iterator {
    public:
      constexpr auto operator++() -> Iterator & {
        bb &= bb - 1;
        return *this;
      }
      constexpr auto operator*() const -> Square { return Square{narrow_cast<u8>(std::countr_zero(bb))}; }
      constexpr auto operator==(const Iterator &) const -> bool = default;

    private:
      friend class Bitboard;
      explicit constexpr Iterator(u128 bb) : bb(bb) {}
      u128 bb;
    };

    constexpr auto begin() const -> Iterator { return Iterator{raw}; }
    constexpr auto end() const -> Iterator { return Iterator{0}; }

  private:
    inline static constexpr u128 mask = (1_u128 << 81) - 1;
    inline static constexpr u128 file_mask = 0x001008040201008040201_u128;
    inline static constexpr u128 rank_mask = 0x1FF_u128;
  }; // namespace lb

  inline constexpr auto operator&(Bitboard a, Bitboard b) -> Bitboard { return Bitboard{a.raw & b.raw}; }
  inline constexpr auto operator|(Bitboard a, Bitboard b) -> Bitboard { return Bitboard{a.raw | b.raw}; }
  inline constexpr auto operator^(Bitboard a, Bitboard b) -> Bitboard { return Bitboard{a.raw ^ b.raw}; }
} // namespace lb

template <> struct std::formatter<lb::Color, char> {
  template <class ParseContext> constexpr auto parse(ParseContext &ctx) -> ParseContext::iterator { return ctx.begin(); }

  template <class FmtContext> auto format(lb::Color c, FmtContext &ctx) const -> FmtContext::iterator {
    return std::format_to(ctx.out(), "{}", c == lb::Color::sente ? 'b' : 'w');
  }
};

template <> struct std::formatter<lb::PieceType, char> {
  template <class ParseContext> constexpr auto parse(ParseContext &ctx) -> ParseContext::iterator { return ctx.begin(); }

  template <class FmtContext> auto format(lb::PieceType ptype, FmtContext &ctx) const -> FmtContext::iterator {
    return std::format_to(ctx.out(), "{}", lb::PieceType::en_strings[0][ptype.raw]);
  }
};

template <> struct std::formatter<lb::Square, char> {
  template <class ParseContext> constexpr auto parse(ParseContext &ctx) -> ParseContext::iterator { return ctx.begin(); }

  template <class FmtContext> auto format(lb::Square sq, FmtContext &ctx) const -> FmtContext::iterator {
    return std::format_to(ctx.out(), "{}{}", static_cast<char>('1' + sq.raw % 9), static_cast<char>('a' + sq.raw / 9));
  }
};

template <> struct std::formatter<lb::Move, char> {
  template <class ParseContext> constexpr auto parse(ParseContext &ctx) -> ParseContext::iterator { return ctx.begin(); }

  template <class FmtContext> auto format(lb::Move m, FmtContext &ctx) const -> FmtContext::iterator {
    if (m.drop()) {
      return std::format_to(ctx.out(), "{}*{}", m.ptype(), m.to());
    } else {
      return std::format_to(ctx.out(), "{}{}{}", m.from(), m.to(), m.promo() ? "+" : "");
    }
  }
};
