#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <compare>
#include <expected>
#include <format>
#include <string_view>
#include <utility>

#include "lb/assert.h"
#include "lb/types.h"

namespace lb {
  enum class ParseError {
    invalid_char,
    invalid_length,
    out_of_range,
    invalid_hand,
    invalid_board,
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

    inline constexpr auto promotable() -> bool { return raw >= pawn && raw <= silver; }
    inline constexpr auto promoted() -> bool { return raw >= tokin; }
    inline constexpr auto promote() -> PieceType {
      lb_assert(raw != none && raw != gold);
      return PieceType{static_cast<Inner>(raw | 0x8)};
    }
    inline constexpr auto demote() -> PieceType { return promoted() ? PieceType{static_cast<Inner>(raw & 0x7)} : *this; }
    inline constexpr auto toBitboardIndex() -> usize {
      lb_assert(raw != none);
      return std::min<usize>(bitboard_count, raw) - 1;
    }

    inline constexpr auto operator==(const PieceType &) const -> bool = default;

    inline static constexpr std::array<std::array<const char *, 2>, 15> en_strings{{
        {"-", "-"},
        {"P", "p"},
        {"B", "b"},
        {"R", "r"},
        {"L", "l"},
        {"N", "n"},
        {"S", "s"},
        {"G", "g"},
        {"K", "k"},
        {"+P", "+p"},
        {"+B", "+b"},
        {"+R", "+r"},
        {"+L", "+l"},
        {"+N", "+n"},
        {"+S", "+s"},
    }};
    inline static constexpr std::array<std::array<const char *, 2>, 15> ja_strings{{
        {"　", "　"},
        {"歩", "歩"},
        {"角", "角"},
        {"飛", "飛"},
        {"香", "香"},
        {"桂", "桂"},
        {"銀", "銀"},
        {"金", "金"},
        {"玉", "王"},
        {"と", "と"},
        {"馬", "馬"},
        {"龍", "龍"},
        {"杏", "杏"},
        {"圭", "圭"},
        {"全", "全"},
    }};
  };

  struct Square {
    u8 raw = 0;

    explicit constexpr Square(u8 raw) : raw(raw) { lb_assert(raw < 81); }

    static constexpr auto parse(std::string_view str) -> std::expected<Square, ParseError> {
      if (str.size() != 2)
        return std::unexpected(ParseError::invalid_length);
      if (str[0] < '1' or str[0] > '9')
        return std::unexpected(ParseError::invalid_char);
      const u8 file = str[0] - '1';
      if (str[1] < 'a' or str[1] > 'i')
        return std::unexpected(ParseError::invalid_char);
      const u8 rank = str[1] - 'a';
      return Square{narrow_cast<u8>(rank * 9 + file)};
    }

    constexpr auto operator<=>(const Square &) const -> std::strong_ordering = default;
  };

  struct Move {
    u16 raw = 0;

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
    constexpr auto drop() -> bool { return (raw & drop_flag) != 0; }
    constexpr auto to() -> Square { return Square{static_cast<u8>(raw & 0x7F)}; }
    constexpr auto promo() -> bool {
      lb_assert(!drop());
      return static_cast<bool>(raw >> 15);
    }
    constexpr auto from() -> Square {
      lb_assert(!drop());
      return Square{static_cast<u8>((raw >> 8) & 0x7F)};
    }
    constexpr auto ptype() -> PieceType {
      lb_assert(drop());
      return PieceType{static_cast<PieceType::Inner>(raw >> 8)};
    }

    inline constexpr auto operator==(const Move &) const -> bool = default;

  private:
    inline static constexpr u16 drop_flag = 1 << 7;
  };

  enum class Direction { n = 0, ne = 1, e = 2, se = 3, s = 4, sw = 5, w = 6, nw = 7 };

  struct Bitboard {
    u128 raw = 0;

    explicit constexpr Bitboard(u128 raw) : raw(raw) {}

    static constexpr auto rank(usize i) -> Bitboard { return Bitboard{0x1FF_u128 << (i * 9)}; }
    static constexpr auto file(usize i) -> Bitboard { return Bitboard{0x001008040201008040201_u128 << i}; }
    static constexpr auto rankRelative(usize i, Color perspective) -> Bitboard {
      switch (perspective) {
      case Color::sente:
        return rank(i);
      case Color::gote:
        return rank(8 - i);
      }
      std::unreachable();
    }

    static constexpr auto fromSq(Square sq) -> Bitboard { return Bitboard{1_u128 << sq.raw}; }
    constexpr auto toSq() const -> Square { return Square{narrow_cast<u8>(std::countr_zero(raw))}; }

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
      return Bitboard{up | down};
    }

    inline constexpr auto shift(Direction dir) const -> Bitboard {
      switch (dir) {
      case Direction::n:
        return Bitboard{raw >> 9};
      case Direction::s:
        return Bitboard{(raw << 9) & mask};
      case Direction::e:
        return Bitboard{(raw & ~file(8).raw) >> 1};
      case Direction::w:
        return Bitboard{((raw & ~file(0).raw) << 1) & mask};
      case Direction::ne:
        return Bitboard{(raw & ~file(8).raw) >> 10};
      case Direction::nw:
        return Bitboard{(raw & ~file(0).raw) >> 8};
      case Direction::se:
        return Bitboard{((raw & ~file(8).raw) << 8) & mask};
      case Direction::sw:
        return Bitboard{((raw & ~file(0).raw) << 10) & mask};
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

    constexpr auto begin() -> Iterator { return Iterator{raw}; }
    constexpr auto end() -> Iterator { return Iterator{0}; }

  private:
    inline static constexpr u128 mask = (1_u128 << 81) - 1;
  }; // namespace lb

  inline constexpr auto operator&(Bitboard a, Bitboard b) -> Bitboard { return Bitboard{a.raw & b.raw}; }
  inline constexpr auto operator|(Bitboard a, Bitboard b) -> Bitboard { return Bitboard{a.raw | b.raw}; }
  inline constexpr auto operator^(Bitboard a, Bitboard b) -> Bitboard { return Bitboard{a.raw ^ b.raw}; }
  inline constexpr auto operator~(Bitboard a) -> Bitboard { return Bitboard{~a.raw}; }
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
    return std::format_to(ctx.out(), "{}", lb::PieceType::en_strings[ptype.raw][0]);
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
