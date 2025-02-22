#pragma once

#include <concepts>
#include <format>
#include <source_location>

namespace lb::internal {
  auto die(std::source_location location, std::string_view expr) -> void;
  auto vdie(std::source_location location, std::string_view expr, std::string_view fmt, std::format_args args) -> void;
  template <typename... Args> inline auto die(std::source_location location, std::string_view expr, std::string_view fmt, Args &&...args) -> void {
    vdie(location, expr, fmt, std::make_format_args(args...));
  }
} // namespace lb::internal

#define lb_assert(expr, ...)                                                                                                                         \
  if (!(expr)) [[unlikely]]                                                                                                                          \
    lb::internal::die(std::source_location::current(), #expr __VA_OPT__(, __VA_ARGS__));

namespace lb {
  template <std::integral Dest> inline auto narrow_cast(std::integral auto src) -> Dest {
    static_assert(sizeof(Dest) < sizeof(decltype(src)));
    lb_assert(static_cast<Dest>(src) == src);
    return static_cast<Dest>(src);
  }
} // namespace lb
