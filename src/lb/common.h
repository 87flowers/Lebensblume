#pragma once

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

  struct Square {
    u8 raw;

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

  struct Bitboard {
    u128 raw;

    explicit Bitboard(u128 raw) : raw(raw) {}

    static auto fromSq(Square sq) -> Bitboard { return Bitboard{u128(1) << sq.raw}; }
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

template <> struct std::formatter<lb::Square, char> {
  template <class ParseContext> constexpr auto parse(ParseContext &ctx) -> ParseContext::iterator { return ctx.begin(); }

  template <class FmtContext> auto format(lb::Square sq, FmtContext &ctx) const -> FmtContext::iterator {
    return std::format_to(ctx.out(), "{}{}", static_cast<char>('1' + sq.raw % 9), static_cast<char>('a' + sq.raw / 9));
  }
};
