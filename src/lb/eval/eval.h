#pragma once

#include <limits>

#include "lb/types.h"
#include "lb/util/assert.h"

namespace lb::eval {

  inline constexpr i32 no_moves = std::numeric_limits<i32>::min();
  inline constexpr i32 min_score = -std::numeric_limits<i32>::max();
  inline constexpr i32 max_score = std::numeric_limits<i32>::max();

  inline constexpr auto mated(i32 ply) -> i32 {
    lb_assert(ply >= 0);
    return -std::numeric_limits<i32>::max() + ply;
  }

  inline constexpr auto mate(i32 ply) -> i32 {
    lb_assert(ply >= 0);
    return std::numeric_limits<i32>::max() - ply;
  }

} // namespace lb::eval
