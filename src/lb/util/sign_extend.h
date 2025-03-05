#pragma once

#include "lb/types.h"

namespace lb {

  template <usize bitsize, typename Dest> inline constexpr auto signExtend(auto src) {
    struct {
      Dest x : bitsize;
    } tmp;
    tmp.x = src;
    return static_cast<Dest>(tmp.x);
  }

} // namespace lb
