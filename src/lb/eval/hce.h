#pragma once

#include "lb/types.h"

namespace lb {
  struct Position;
}

namespace lb::eval {

  auto hce(const Position &position) -> i32;

} // namespace lb::eval
