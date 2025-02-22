#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <compare>
#include <expected>
#include <format>
#include <string_view>

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
    sente,
    gote,
  };

  inline auto invert(Color c) -> Color { return c == Color::sente ? Color::gote : Color::sente; }

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

    /* implicit */ PieceType(Inner raw) : raw(raw) {}

    inline auto promotable() -> bool { return raw >= pawn && raw <= silver; }
    inline auto promoted() -> bool { return raw >= tokin; }
    inline auto promote() -> PieceType {
      lb_assert(raw != none && raw != gold);
      return PieceType{static_cast<Inner>(raw | 0x8)};
    }
    inline auto demote() -> PieceType { return promoted() ? PieceType{static_cast<Inner>(raw & 0x7)} : *this; }
    inline auto toBitboardIndex() -> usize {
      lb_assert(raw != none);
      return std::min<usize>(bitboard_count, raw) - 1;
    }

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

  inline auto operator==(PieceType a, PieceType b) -> bool { return a.raw == b.raw; }

  struct Square {
    u8 raw = 0;

    explicit Square(u8 raw) : raw(raw) { lb_assert(raw < 81); }

    static auto parse(std::string_view str) -> std::expected<Square, ParseError> {
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
  };

  inline auto operator<=>(Square a, Square b) -> std::strong_ordering { return a.raw <=> b.raw; }

  struct Move {
    u16 raw = 0;

    explicit constexpr Move(u16 raw) : raw(raw) {}

    static constexpr Move none() { return Move{0}; };

    static auto makeMove(Square from, Square to, bool promotion) -> Move {
      u16 result = 0;
      result |= to.raw;
      result |= static_cast<u16>(from.raw) << 8;
      result |= static_cast<u16>(promotion) << 15;
      return Move{result};
    }

    static auto makeDrop(PieceType ptype, Square to) -> Move {
      u16 result = drop_flag;
      result |= to.raw;
      result |= static_cast<u16>(ptype.raw) << 8;
      return Move{result};
    }
    auto drop() -> bool { return (raw & drop_flag) != 0; }
    auto to() -> Square { return Square{static_cast<u8>(raw & 0x7F)}; }
    auto promo() -> bool {
      lb_assert(!drop());
      return static_cast<bool>(raw >> 15);
    }
    auto from() -> Square {
      lb_assert(!drop());
      return Square{static_cast<u8>((raw >> 8) & 0x7F)};
    }
    auto ptype() -> PieceType {
      lb_assert(drop());
      return PieceType{static_cast<PieceType::Inner>(raw >> 8)};
    }

  private:
    inline static constexpr u16 drop_flag = 1 << 7;
  };

  inline auto operator==(Move a, Move b) -> bool { return a.raw == b.raw; }

  struct Bitboard {
    u128 raw = 0;

    explicit Bitboard(u128 raw) : raw(raw) {}

    static constexpr auto rank(usize i) -> Bitboard { return Bitboard{0x1FF_u128 << (i * 9)}; }

    static auto fromSq(Square sq) -> Bitboard { return Bitboard{1_u128 << sq.raw}; }
    auto toSq() const -> Square { return Square{narrow_cast<u8>(std::countr_zero(raw))}; }

    auto operator|=(Bitboard b) -> Bitboard & {
      raw |= b.raw;
      return *this;
    }

    auto operator^=(Bitboard b) -> Bitboard & {
      raw ^= b.raw;
      return *this;
    }

    auto operator&=(Bitboard b) -> Bitboard & {
      raw &= b.raw;
      return *this;
    }
  };

  inline auto operator&(Bitboard a, Bitboard b) -> Bitboard { return Bitboard{a.raw & b.raw}; }
  inline auto operator|(Bitboard a, Bitboard b) -> Bitboard { return Bitboard{a.raw | b.raw}; }
  inline auto operator^(Bitboard a, Bitboard b) -> Bitboard { return Bitboard{a.raw ^ b.raw}; }
  inline auto operator~(Bitboard a) -> Bitboard { return Bitboard{~a.raw}; }
  inline auto operator==(Square a, Square b) -> bool { return a.raw == b.raw; }
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
