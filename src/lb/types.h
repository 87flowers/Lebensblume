#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

namespace lb {
  using u8 = std::uint8_t;
  using u16 = std::uint16_t;
  using u32 = std::uint32_t;
  using u64 = std::uint64_t;
  using u128 = unsigned __int128;

  using i8 = std::int8_t;
  using i16 = std::int16_t;
  using i32 = std::int32_t;
  using i64 = std::int64_t;
  using i128 = __int128;

  using usize = std::size_t;

  using f32 = float;
  using f64 = double;

  namespace time {
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using Duration = TimePoint::duration;
    using FloatSeconds = std::chrono::duration<double>;
    using Milliseconds = std::chrono::duration<i64, std::milli>;

    template <typename T> constexpr auto cast(const auto &d) -> T { return std::chrono::duration_cast<T>(d); }
  } // namespace time

  namespace internal {
    inline constexpr auto digitValue(char c) -> u8 {
      if (c >= '0' && c <= '9')
        return c - '0';
      if (c >= 'a' && c <= 'z')
        return c - 'a' + 0xA;
      if (c >= 'A' && c <= 'Z')
        return c - 'A' + 0xA;
      throw c;
    }

    template <typename T, T base> inline constexpr auto parse(const char *x) -> T {
      T result = 0;
      while (*x) {
        const T digit = digitValue(*x);
        if (digit >= base)
          throw x;
        T next_result = result * base + digit;
        if (next_result < result)
          throw "integer overflow";
        x++;
      }
      return result;
    }

    template <typename T> inline constexpr auto parseWithPrefix(const char *x) -> T {
      if (x == nullptr)
        return 0;
      if (x[0] == '0') {
        if (x[1] == 'b')
          return parse<T, 2>(x + 2);
        if (x[1] == 'x')
          return parse<T, 16>(x + 2);
        if (x[1] != '\0')
          throw x; // We do not support octal
      }
      return parse<T, 10>(x);
    }
  } // namespace internal

  inline constexpr auto operator""_u128(const char *x) -> u128 { return internal::parseWithPrefix<u128>(x); }

} // namespace lb
