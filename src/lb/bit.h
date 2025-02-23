#pragma once

#include <immintrin.h>

#include "lb/types.h"

namespace lb {

  constexpr auto pextSlow(u64 x, u64 m) -> u64 {
    u64 result = 0;
    for (u64 b = 1; m != 0; b += b) {
      if (x & m & -m != 0)
        result |= b;
      m &= m - 1;
    }
    return result;
  }

  constexpr auto pext(u64 x, u64 m) -> u64 {
    if (std::is_constant_evaluated())
      return pextSlow(x, m);
    return _pext_u64(x, m);
  }

  template <typename T> constexpr auto pinc(T x, T m) -> T { return static_cast<T>(x - m) & m; }

} // namespace lb
